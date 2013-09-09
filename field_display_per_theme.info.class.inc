<?php

/**
 * @file
 * Definition of the FieldDisplayPerThemeFieldInfo class.
 */

/**
 * Provides field and instance definitions for the current runtime environment.
 *
 * We alter the display configuration returned so that our per-theme overrides
 * are honoured.
 *
 * This, admittedly, is an extremely hacky way to override field configuration,
 * and we ONLY do it to get our ADMIN forms to work properly.
 *
 * For our display-time we just use hook_field_display_alter().
 */
class FieldDisplayPerThemeFieldInfo extends FieldInfo {

  protected $fieldDisplayPerThemeOverrides = array();
  protected $fieldDisplayPerThemeViewMode = NULL;

  /**
   * Call this to set the field display overrides.
   */
  public function setDisplayOverrides($entity_type, $bundle, $view_mode, $overrides) {
    $this->fieldDisplayPerThemeOverrides[$entity_type][$bundle][$view_mode] = $overrides;
  }

  /**
   * Call this to set the field display overrides.
   */
  public function setActiveViewMode($view_mode) {
    $this->fieldDisplayPerThemeViewMode = $view_mode;
  }

  public function getInstances($entity_type = NULL) {
    if ($return = parent::getInstances($entity_type)) {
      // Normalize the result for easy iteration.
      $all_instances = &$return;
      if (isset($entity_type)) {
        $all_instances = array($entity_type => &$return);
      }
      foreach ($all_instances as $_entity_type => &$bundle_instances) {
        foreach ($bundle_instances as $bundle_name => &$instances) {
          foreach ($instances as &$instance) {
            $this->applyFieldInstanceOverrides($_entity_type, $bundle_name, $instance);
          }
        }
      }
    }
    return $return;
  }

  public function getBundleInstances($entity_type, $bundle) {
    if ($instances = parent::getBundleInstances($entity_type, $bundle)) {
      foreach ($instances as &$instance) {
        $this->applyFieldInstanceOverrides($entity_type, $bundle, $instance);
      }
    }
    return $instances;
  }

  protected function applyFieldInstanceOverrides($entity_type, $bundle, &$instance) {
    // Make sure our 'active' view mode is present in the results.
    // (This is so we can still access view modes UI to override them when they
    // are not enabled by default).
    if (isset($this->fieldDisplayPerThemeViewMode)) {
      $view_mode = $this->fieldDisplayPerThemeViewMode;
      if (!isset($instance['display'][$view_mode])) {
        if (isset($instance['display']['default'])) {
          $instance['display'][$view_mode] = $instance['display']['default'];
        }
        else {
          $field_map = field_info_field_map();
          if (isset($field_map[$instance['field_name']]['type'])) {
            $field_type = $field_map[$instance['field_name']]['type'];
            $field_type_info = field_info_field_types($field_type);
            $instance['display'][$view_mode] =  array(
              'label' => 'above',
              'type' => $field_type_info['default_formatter'],
              'settings' => array(),
              'weight' => 0,
            );
          }
          else {
            $instance['display'][$view_mode] =  array(
              'label' => 'hidden',
              'type' => '',
              'settings' => array(),
              'weight' => 0,
            );
          }
        }
      }
    }
    // Now apply our overrides.
    if (isset($this->fieldDisplayPerThemeOverrides[$entity_type][$bundle])) {
      foreach ($this->fieldDisplayPerThemeOverrides[$entity_type][$bundle] as $view_mode => $fields) {
        foreach ($fields as $field_name => $display_override) {
          if ($instance['field_name'] == $field_name) {
            $instance['display'][$view_mode] = $display_override + $instance['display'][$view_mode];
          }
        }
      }
    }
  }


  public function getBundleExtraFields($entity_type, $bundle) {
    $extra_fields = parent::getBundleExtraFields($entity_type, $bundle);

    // Override display configuration

    return $extra_fields;
  }


}