<?php

/**
 * @file Field admin pages.
 */

/**
 * Page callback for *our* version of the default view mode page.
 *
 * We don't allow overrides to the default view mode, as that would be too far-
 * reaching an override to make.
 *
 * Instead we display a table of all the view modes and whether or not they are
 * enabled for custom settings by default, and/or for our overrides.
 */
function field_display_per_theme_display_overview_list_form($form, &$form_state, $theme_key, $entity_type, $bundle) {

  $form['entity_type'] = array(
    '#type' => 'value',
    '#value' => $entity_type,
  );
  $form['bundle'] = array(
    '#type' => 'value',
    '#value' => $bundle,
  );

  $form['modes'] = array(
    '#type' => 'fieldset',
    '#title' => t('Overriden view modes'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
    '#theme' => 'field_display_per_theme_overview_list',
  );

  // Collect options and default values for the 'Custom display settings'
  // checkboxes.
  $entity_info = entity_get_info($entity_type);
  $view_modes = $entity_info['view modes'];
  $view_modes_settings = field_view_mode_settings($entity_type, $bundle);
  $view_modes_overrides = field_display_per_theme_config_load($theme_key, $entity_type, $bundle);
  $all_view_modes_force_override = variable_get('field_display_per_theme_force_overrides', array());
  $view_modes_force_override = array();
  if (isset($all_view_modes_force_override[$entity_type][$bundle])) {
    $view_modes_force_override = $all_view_modes_force_override[$entity_type][$bundle];
  }

  $form['modes']['#view_modes'] = $view_modes;
  $form['modes']['#view_modes_settings'] = $view_modes_settings;
  $form['modes']['#view_modes_overrides'] = $view_modes_overrides;

  $form['modes']['#header'][] = array(
    'data' => '',
    'cell_key' => 'enabled',
  );
  $form['modes']['#header'][] = array(
    'data' => t('View mode'),
    'cell_key' => 'label',
  );
  $form['modes']['#header'][] = array(
    'data' => t('Field config status'),
    'cell_key' => 'defaultly_enabled',
  );
  $form['modes']['#header'][] = array(
    'data' => t('Field override status'),
    'cell_key' => 'overrides_present',
  );

  foreach ($view_modes as $view_mode_name => $view_mode_info) {
    $defaultly_enabled = !empty($view_modes_settings[$view_mode_name]['custom_settings']);
    $overrides_present = !empty($view_modes_overrides[$view_mode_name]);
    $overrides_forced = !empty($view_modes_force_override[$view_mode_name]);

    $form['modes'][$view_mode_name] = array(
      'name' => array(
        '#type' => 'value',
        '#value' => $view_mode_name,
      ),
    );
    $form['modes'][$view_mode_name]['enabled'] = array(
      '#type' => 'checkbox',
      '#title' => t('Enable custom overrides'),
      '#title_display' => 'none',
      '#parents' => array('modes', $view_mode_name),
      '#default_value' => $overrides_forced || $overrides_present,
      '#disabled' => $overrides_present,
    );
    $form['modes'][$view_mode_name]['label'] = array(
      '#markup' => check_plain($view_mode_info['label']),
    );
    $form['modes'][$view_mode_name]['defaultly_enabled'] = array(
      '#type' => 'item',
      '#title' => t('Field config status'),
      '#title_display' => 'none',
      '#markup' => $defaultly_enabled ? t('Custom settings') : t('Uses <em>Default</em> view mode'),
    );
    $form['modes'][$view_mode_name]['overrides_present'] = array(
      '#type' => 'item',
      '#title' => t('Field override status'),
      '#title_display' => 'none',
      '#markup' => $overrides_present ? t('Overrides present') : t('No overrides'),
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Submit handler for field_display_per_theme_display_overview_list_form().
 *
 * @param $form
 * @param $form_state
 */
function field_display_per_theme_display_overview_list_form_submit($form, &$form_state) {

  $entity_type = $form_state['values']['entity_type'];
  $bundle = $form_state['values']['bundle'];

  $view_modes_force_overrides = variable_get('field_display_per_theme_force_overrides', array());
  $view_modes_force_overrides[$entity_type][$bundle] = array_filter($form_state['values']['modes']);
  variable_set('field_display_per_theme_force_overrides', $view_modes_force_overrides);

  drupal_set_message(t('Your settings have been saved'));
}

/**
 * Themes the view mode overview list page into a table.
 *
 * @param $vars
 *
 * @return string
 */
function theme_field_display_per_theme_overview_list($vars) {

  $el = $vars['element'];

  $output = array(
    '#theme' => 'table',
    '#header' => $el['#header'],
    '#rows' => array(),
  );

  foreach ($el['#view_modes'] as $view_mode_name => $view_mode) {
    if (isset($el[$view_mode_name])) {
      $cells = array();
      foreach ($el['#header'] as $header_info) {
        if (isset($header_info['cell_key']) && isset($el[$view_mode_name][$header_info['cell_key']])) {
          $cells[] = array(
            'data' => drupal_render($el[$view_mode_name][$header_info['cell_key']]),
          );
        }
      }
      $output['#rows'][] = array(
        'data' => $cells,
      );
    }
  }

  return drupal_render($output);
}

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

  $form_state = array();
  $form_state['build_info']['args'] = array($entity_type, $bundle_arg, $view_mode);

  $form_state['field_display_per_theme'] = array(
    'theme' => $theme,
    // Our override data.
    'overrides' => field_display_per_theme_config_load($theme->name, $entity_type, $bundle_arg, $view_mode),
  );
  // We want to just invoke the standard field_ui Manage Display form, but in
  // order to maintain control over the field config from which it builds itself
  // we actually build a form which wraps it with our own wrapper callback.
  //   see our own hook_forms() implementation.
  //   see field_display_per_theme_display_overview_form_wrapper() below.
  return drupal_build_form('field_display_per_theme_display_overview_form', $form_state);
}

/**
 * Our replacement for field_ui_display_overview_form().
 *
 * All we do is seed a few things and then defer to the standard constructor.
 *
 * @param $form
 * @param $form_state
 * @param $entity_type
 * @param $bundle
 * @param $view_mode
 * @return array
 */
function field_display_per_theme_display_overview_form($form, &$form_state, $entity_type, $bundle, $view_mode) {

  // We need to make sure the form is built using our override display values
  // (where overrides have been specified). So we swap our the FieldInfo cache
  // object for the rest of the form build to our own magic one =).
  $field_info_cache_object = &drupal_static('_field_info_field_cache');
  $field_info_cache_object = new FieldDisplayPerThemeFieldInfo();
  $field_info_cache_object->setDisplayOverrides($entity_type, $bundle, $view_mode, $form_state['field_display_per_theme']['overrides']);
  $field_info_cache_object->setActiveViewMode($view_mode);

  form_load_include($form_state, 'admin.inc', 'field_ui');
  return field_ui_display_overview_form($form, $form_state, $entity_type, $bundle, $view_mode);
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
function _field_display_per_theme_display_overview_form_alter(&$form, &$form_state) {

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

      $is_overridden = isset($form_state['field_display_per_theme']['overrides'][$field_name]);

      $field_element['field_display_per_theme_status'] = array(
        '#type' => 'value',
        '#value' => $is_overridden,
      );

      if (!$is_overridden) {
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
          '#op' => $is_overridden ? 'lock' : 'unlock',
          '#value' => $is_overridden ? t('Delete override') : t('Override'),
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
  // Field UI javascript doesn't work without tabledrag.
  $form['#attached']['js'] = array_diff($form['#attached']['js'], array(drupal_get_path('module', 'field_ui') . '/field_ui.js'));
  // Refresh button is obsolete without javascript.
  unset($form['refresh']);
}

/**
 * Form submission handler for field_display_per_theme_display_overview_form().
 */
function field_display_per_theme_display_overview_form_submit($form, &$form_state) {
  $form_values = $form_state['values'];
  $entity_type = $form['#entity_type'];
  $bundle = $form['#bundle'];
  $view_mode = $form['#view_mode'];
  $theme = $form_state['field_display_per_theme']['theme'];

  $display = array();

  // Save data for 'regular' fields.
  foreach ($form['#fields'] as $field_name) {
    $values = $form_values['fields'][$field_name];
    if (!empty($values['field_display_per_theme_status'])) {
      // Get formatter settings. They lie either directly in submitted form
      // values (if the whole form was submitted while some formatter
      // settings were being edited), or have been persisted in
      // $form_state.
      $settings = array();
      if (isset($values['settings_edit_form']['settings'])) {
        $settings = $values['settings_edit_form']['settings'];
      }
      elseif (isset($form_state['formatter_settings'][$field_name])) {
        $settings = $form_state['formatter_settings'][$field_name];
      }

      // Only save settings actually used by the selected formatter.
      $default_settings = field_info_formatter_settings($values['type']);
      $settings = array_intersect_key($settings, $default_settings);

      $display[$field_name] = array(
        'label' => $values['label'],
        'type' => $values['type'],
        'weight' => $values['weight'],
        'settings' => $settings,
      );
    }
  }

  // Save data for 'extra' fields.
  foreach ($form['#extra'] as $name) {
    $display[$name] = array(
      'weight' => $form_values['fields'][$name]['weight'],
      'visible' => $form_values['fields'][$name]['type'] == 'visible',
    );
  }

  field_display_per_theme_config_delete($theme->name, $entity_type, $bundle, $view_mode);
  field_display_per_theme_config_save($theme->name, $entity_type, $bundle, $view_mode, $display);

  drupal_set_message(t('Your overrides have been saved.'));
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