<?php

/**
 * @file Field admin pages.
 */

/**
 * Page callback for *our* version of the Manage Display field UI.
 *
 * This is not a form constructor, but just a menu callback.
 *
 */
function field_display_per_theme_display_overview_page($theme_key, $entity_type, $bundle_arg, $view_mode) {

  $themes = list_themes();
  if (isset($themes[$theme_key])) {
    $theme = $themes[$theme_key];
  }
  else {
    return MENU_NOT_FOUND;
  }

  // We make ourselves exactly the same as the standard Manage Display form,
  // but with a special flag in $form_state so we can make our own alterations.
  // See _field_display_per_theme_form_field_ui_display_overview_form_alter()
  // below.
  $form_state = array();
  $form_state['build_info']['args'] = array($entity_type, $bundle_arg, $view_mode);

  $form_state['field_display_per_theme'] = array(
    'theme' => $theme,
    // Our override data.
    'overrides' => field_display_per_theme_config_load($entity_type, $bundle_arg, $view_mode),
  );
  // We want to just invoke the standard field_ui Manage Display form, but in
  // order to maintain control over the field config from which it builds itself
  // we actually build a form which wraps it with our own wrapper callback.
  //   see our own hook_forms() implementation.
  //   see field_display_per_theme_display_overview_form_wrapper() below.
  form_load_include($form_state, 'admin.inc', 'field_ui');
  return drupal_build_form('field_display_per_theme_display_overview_form', $form_state);
}

/**
 * A custom form wrapper on the field_ui_display_overview_form, but only when
 * building our special version of it.
 *
 * @param $form
 * @param $form_state
 */
function field_display_per_theme_display_overview_form_wrapper($form, &$form_state, $entity_type, $bundle, $view_mode) {

  // We need to make sure the form is built using our override display values
  // (where overrides have been specified). So we swap our the FieldInfo cache
  // object for the rest of the form build to our own magic one =).
  $field_info_cache_object = &drupal_static('_field_info_field_cache');
  $field_info_cache_object = new FieldDisplayPerThemeFieldInfo();
  $field_info_cache_object->setDisplayOverrides($entity_type, $bundle, $view_mode, $form_state['field_display_per_theme']['overrides']);

  return $form;
}

/**
 * Implements hook_form_FORM_ID_alter().
 *
 * Well, it doesn't actually implement it directly (see .module file), but to
 * keep code tidy we keep the contents of our hook implementation here.
 *
 * @param $form
 * @param $form_state
 */
function _field_display_per_theme_form_field_ui_display_overview_form_alter(&$form, &$form_state) {

  // For consistency we try to make our UI as similar to the original Manage
  // Display UI as possible. It differs from the original as follows:
  //
  //  1. Lock all fields by default - allow the user to opt-in to overriding,
  //
  //  2. Remove javascript table-drag, so user has more control over overriding
  //     weights to what they need to be to 'slot' in-between the fields they
  //     are not overriding.

  $field_keys = array_merge($form['#fields'], $form['#extra']);

  // 1. Lock all fields not being overridden.
  if (!empty($field_keys))
  foreach ($field_keys as $field_name) {
    if (isset($form['fields'][$field_name])) {
      $field_element = &$form['fields'][$field_name];

      $is_locked = !isset($form_state['field_display_per_theme']['overrides'][$field_name]);

      if ($is_locked) {
        // Prevents Field UI's javascript attaching.
        unset($field_element['#js_settings']);
        // Disable all form elements within this field.
        _field_display_per_theme_element_modify_recursive($field_element,
                                                          array('#type'),
                                                          array('#disabled' => TRUE),
                                                          array('#ajax'));
        unset($field_element['#disabled']);
      }

      // Add unlock/lock button next to field name.
      $field_element['human_name'] = array(
        'wrapped' => $field_element['human_name'],
        'field_display_per_theme_override' => array(
          '#type' => 'submit',
          '#field_name' => $field_name,
          '#is_extra_field' => in_array($field_name, $form['#extra']),
          '#name' => 'field_display_per_theme_' . $field_name . '_lock_toggle',
          '#op' => $is_locked ? 'unlock' : 'lock',
          '#value' => $is_locked ? t('Override') : t('Delete override'),
          '#submit' => array('field_display_per_theme_lock_toggle_submit'),
          // Cheeky way of getting our button to look alright..
          '#attributes' => array(
            'class' => array(
              'field-formatter-settings-edit',
            ),
          ),
        ),
      );
      unset($field_element);
    }
  }

  // 2. Remove javascript table-drag.
  unset($form['#attached']['drupal_add_tabledrag']);

  dpm(func_get_args(), __FUNCTION__);
}


/**
 * Form submission handler for buttons in field_ui_display_overview_form().
 */
function field_display_per_theme_lock_toggle_submit($form, &$form_state) {
  $trigger = $form_state['triggering_element'];
  $op = $trigger['#op'];
  $field_name = $trigger['#field_name'];

  switch ($op) {
    case 'unlock':
      // Start override display config at the default value.
      $entity_type = $form['#entity_type'];
      $bundle = $form['#bundle'];
      $view_mode = $form['#view_mode'];
      if ($trigger['#is_extra_field']) {
        $extra_fields = field_info_extra_fields($entity_type, $bundle, 'display');
        $display = $extra_fields[$field_name]['display'][$view_mode];
      }
      else {
        $instances = field_info_instances($entity_type, $bundle);
        $display = $instances[$field_name]['display'][$view_mode];
      }
      $form_state['field_display_per_theme']['overrides'][$field_name] = $display;
      break;

    case 'lock':
      unset($form_state['field_display_per_theme']['overrides'][$field_name]);
      break;
  }

  $form_state['rebuild'] = TRUE;
}

/**
 * Helper function which recursively scans $elements, and all their children,
 * finding which match the properties in $match, overriding their properties
 * with those in $overwrite.
 *
 * @param array $elements
 * @param array $with_keys
 * @param array $overwrite
 * @param array $unset
 *
 * @return bool
 *   TRUE if some child element was found & changed, FALSE otherwise.
 */
function _field_display_per_theme_element_modify_recursive(array &$elements, array $with_keys, array $overwrite, array $unset) {

  $match_made = FALSE;

  // Find if we match.
  $with_keys_assoc = array_fill_keys($with_keys, 1);
  if (empty($with_keys) || (count(array_intersect_key($elements, $with_keys_assoc)))) {
    $elements = $overwrite + $elements;
    $elements = array_diff_key($elements, array_fill_keys($unset, 1));
    $match_made = TRUE;
  }

  // Test our children
  foreach (element_children($elements) as $k) {
    $match_made |= _field_display_per_theme_element_modify_recursive($elements[$k], $with_keys, $overwrite, $unset);
  }

  return $match_made;
}