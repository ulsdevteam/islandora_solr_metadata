<?php
/**
 * @file
 * Theme functions for the islandora_solr_metadata module.
 */

/**
 * Implements theme_element().
 */
function theme_islandora_solr_metadata_management_table(array $variables) {
  $table = $variables['table'];
  $rows = array();
  $weight_class = 'islandora-solr-metadata-weight';
  foreach (element_children($table['rows']) as $id) {
    $row = &$table['rows'][$id];
    $row['weight']['#attributes'] = array('class' => array($weight_class));
    $rows[] = array(
      'data' => array(
        drupal_render($row['remove_field']),
        drupal_render($row['solr_field_markup']),
        drupal_render($row['operations']),
        drupal_render($row['weight']),
      ),
      'class' => array('draggable'),
    );
  }
  $table_id = 'entry-order';
  $output = theme('table', array(
    'header' => $table['#header'],
    'rows' => $rows,
    'attributes' => array('id' => $table_id),
    'empty' => t('There are no entries available.')));
  $output .= drupal_render_children($table);
  drupal_add_tabledrag($table_id, 'order', 'sibling', $weight_class);
  return $output;
}

/**
 * Prepares variables for islandora_solr_metadata_display templates.
 *
 * Default template: islandora-solr-metadata-display.tpl.php
 *
 * @param array $variables
 *   An associative array containing:
 *   - islandora_object: The AbstractObject for which we are generating a
 *     metadata display.
 *   - print: A boolean indicating to disable some functionality, to facilitate
 *     printing. When TRUE, avoids adding the "collapsible" and "collapsed"
 *     classes to fieldsets.
 */
function template_preprocess_islandora_solr_metadata_display(array &$variables) {
  module_load_include('inc', 'islandora_solr_metadata', 'includes/db');
  module_load_include('inc', 'islandora', 'includes/utilities');
  drupal_add_js('misc/form.js');
  drupal_add_js('misc/collapse.js');

  $object = $variables['islandora_object'];
  $db_fields = array();
  $solr_fields =& $variables['solr_fields'];
  $associations = $variables['associations'];

  foreach ($associations as $configuration_id) {
    $field = islandora_solr_metadata_get_fields($configuration_id['configuration_id']);
    $db_fields = array_merge($db_fields, $field);
  }
  foreach ($db_fields as $solr_field => $value) {
    if (isset($solr_fields[$solr_field])) {
      continue;
    }
    // Make an array for use later on.
    $solr_fields[$solr_field] = $value + array(
      'value' => array(),
    );
  }
  $variables['parent_collections'] = islandora_get_parents_from_rels_ext($object);
}

/**
 * Implements template_process_HOOK() for islandora_solr_metadata_description.
 */
function template_process_islandora_solr_metadata_display(array &$variables) {
  $variables['found'] = islandora_solr_metadata_query_fields($variables['islandora_object'], $variables['solr_fields']);
  // Remove empty values.
  if ($variables['found']) {
    foreach ($variables['solr_fields'] as $solr_field=>$solr_field_array) {
      if (isset($solr_field_array['value']) && is_array($solr_field_array['value'])) {
        foreach ($solr_field_array['value'] as $k=>$val) {
          if (trim($val) == '') {
            unset($variables['solr_fields'][$solr_field]['value'][$k]);
          }
        }
        // If no values left, also remove the field / label.
        if (count($solr_field_array['value']) < 1) {
          unset($variables['solr_fields'][$solr_field]);
        }
      }
    }
  }
  $variables['not_found_message'] = t('Document not present in Solr while generating the metadata display for @id. Try !reloading the page, or contact an administrator if this issue persists.', array(
    '@id' => $variables['islandora_object']->id,
    '!reloading' => '<a href="" onclick="window.location.reload(true)">' . t('reloading') . '</a>',
  ));
}

/**
 * Prepares variables for islandora_solr_metadata_description templates.
 *
 * Default template: islandora-solr-metadata-description.tpl.php
 *
 * @param array $variables
 *   An associative array containing:
 *   - islandora_object: The AbstractObject for which we are generating a
 *     metadata display.
 */
function template_preprocess_islandora_solr_metadata_description(array &$variables) {
  $object = $variables['islandora_object'];
  $solr_fields =& $variables['solr_fields'];
  $associations = $variables['associations'];

  $descriptions = array();
  module_load_include('inc', 'islandora_solr_metadata', 'includes/db');
  foreach ($associations as $configuration_id) {
    $description = islandora_solr_metadata_retrieve_description($configuration_id['configuration_id']);
    if ($description['description_field'] !== NULL) {
      $descriptions[] = $description;
    }
  }
  foreach ($descriptions as $description) {
    $desc_field = $description['description_field'];
    $solr_fields[$desc_field] = array(
      'display_label' => $description['description_label'],
      'value' => array(),
      'description_data' => $description['description_data'],
    );
  }
}

/**
 * Implements template_process_HOOK() for islandora_solr_metadata_description.
 */
function template_process_islandora_solr_metadata_description(array &$variables) {
  $variables['found'] = islandora_solr_metadata_query_fields($variables['islandora_object'], $variables['solr_fields']);

  // XXX: Seems a little odd, yes... Was reusing the "description" value,
  // though.
  $variables['description'] = $variables['solr_fields'];
  // Truncate the description value(s).
  foreach ($variables['description'] as &$field) {
    if (isset($field['description_data']['truncation'])) {
      if ($field['description_data']['truncation']['max_length'] > 0) {
        module_load_include('inc', 'islandora_solr', 'includes/utilities');
        $field['value'] = array_filter($field['value']);
        if ($field['description_data']['truncation']['truncation_type'] == 'whole_field_option') {
          $updated_value[] = islandora_solr_truncate_field_display(
            $field['value'],
            $field['description_data']['truncation']['max_length'],
            $field['description_data']['truncation']['ellipsis'],
            $field['description_data']['truncation']['word_safe'],
            $field['description_data']['truncation']['min_wordsafe_length'],
            "\n"
          );
          $field['value'] = $updated_value;
        }
        else {
          foreach ($field['value'] as $index => $value) {
            $field['value'][$index] = islandora_solr_truncate_field_display(
              (array) $value,
              $field['description_data']['truncation']['max_length'],
              $field['description_data']['truncation']['ellipsis'],
              $field['description_data']['truncation']['word_safe'],
              $field['description_data']['truncation']['min_wordsafe_length'],
              " "
            );
          }
        }
      }
    }
  }
}

/**
 * Perform the query to Solr to get our metadata values.
 *
 * @param AbstractObject $object
 *   The object for which we are going to query Solr... Really only need the
 *   PID...
 * @param array $solr_fields
 *   An associative array mapping Solr fields to associative arrays, of which
 *   we use:
 *   - hyperlink: An optional boolean indicating if the given value should be
 *     rendered a link.
 *   - weight: An optional weight we will use to sort the values.
 *   - formatter: An optional callable to which we will call with each value
 *     from the given field, to return alternate text or markup as desired.
 *
 * @return bool
 *   TRUE if we have found a document for the object in question; otherwise,
 *   FALSE.
 */
function islandora_solr_metadata_query_fields($object, &$solr_fields) {
  module_load_include('inc', 'islandora_solr', 'includes/utilities');
  if (!empty($solr_fields)) {
    $query_processor = new IslandoraSolrQueryProcessor();
    $query_processor->solrQuery = 'PID:"' . $object->id . '"';
    $query_processor->solrParams['fl'] = '*';
    $query_processor->executeQuery(FALSE);
    if ($query_processor->islandoraSolrResult['response']['numFound'] > 0) {
      $solr_results_doc = $query_processor->islandoraSolrResult['response']['objects']['0']['solr_doc'];
      // Ensure we only provide results for what is specified.
      $constrained_results = array_intersect_key($solr_results_doc, $solr_fields);

      foreach ($constrained_results as $solr_field => $value) {
        // XXX. Need to pass $solr_fields by reference here even though it's not
        // modified to get around a bug in PHP 5.3.3 when used on CentOS 6.6,
        // otherwise the calling function doesn't maintain a reference to the
        // original value and it becomes a copy.
        $get_display_value = function ($original_value) use ($solr_field, &$solr_fields, $object) {
          $value = $original_value;
          $field_config = (is_array($solr_fields[$solr_field]) ? $solr_fields[$solr_field] : array()) + array(
            'hyperlink' => 0,
            'formatter' => NULL,
          );
          if (($truncation = drupal_array_get_nested_value($field_config, array('truncation'))) && $truncation['max_length'] > 0 && (!isset($truncation['truncation_type']) || $truncation['truncation_type'] == 'separate_value_option')) {
            $value = truncate_utf8($value, $truncation['max_length'], $truncation['word_safe'], $truncation['ellipsis'], $truncation['min_wordsafe_length']);
            if ($value !== $original_value) {
              $mod_path = drupal_get_path('module', 'islandora_solr');
              drupal_add_js("$mod_path/js/truncation-toggle.js");
              // XXX: implode()'ing to avoid newlines which get converted to
              // <br/>'s in the later check_markup.
              return implode('', array(
                '<span class="toggle-wrapper">',
                t("<span>!value <a href='#' class='toggler'>Show more</a></span>", array(
                  '!value' => $value,
                )),
                t("<span>!original_value <a href='#' class='toggler'>Show less</a></span>", array(
                  '!original_value' => $original_value,
                )),
                '</span>',
              ));
            }
          }
          if (is_callable($field_config['formatter'])) {
            $value = call_user_func($field_config['formatter'], $value);
          }

          if ($field_config['hyperlink'] == 1 && trim($value)) {
            $use_value = $value;
            if ($link_options = drupal_array_get_nested_value($field_config, array('link_options'))) {
              if (isset($link_options['link_text']) && $link_options['link_text'] == 'otherfield') {
                $target_object = islandora_object_load(str_replace('info:fedora/', '', $value)); 
                if (is_object($target_object)) {
                  $site = _get_site_from_config();
                  $target_sites_rel = $target_object->relationships->get(NULL, 'isMemberOfSite');
                  $target_sites = array();
                  foreach ($target_sites_rel as $tar) {
                    if (isset($tar['object']['value'])) {
                      $target_sites[] = 'info:fedora/' . $tar['object']['value'];
                    }
                  }
                  if ($site <> '' && array_search($site, $target_sites) === FALSE) {
                    return ;
                  }
                  $use_target = (isset($link_options['link_target']) && $link_options['link_target'] == 'pathauto') ?
                    islandora_solr_custom_path_replacement($target_object->id,
                                                           $link_options['link_target_options']['link_target_field'],
                                                           $link_options['link_target_options']['link_target_pattern']) :
                    'islandora/object/' . $target_object->id;

                  $use_value = ($link_options['link_text_options']['link_text_text']) ?
                    _get_SOLR_field_value($target_object->id, $link_options['link_text_options']['link_text_text']) : $target_object->label;
                }
              }
              if (isset($link_options['link_target']) && $link_options['link_target'] <> 'SOLR') {
                // Based on the config, see what the target is.
                return ($use_target) ? l($use_value, $use_target) : $use_value;
              }
            }
            if (variable_get('islandora_solr_metadata_omit_empty_values', FALSE) && !trim($value)) {
              // Avoid generating markup (throwing off the "empty" check).
              // Will get stripped out later.
              return;
            }
            $solr_query = format_string('!field:!value', array(
              '!field' => $solr_field,
              '!value' => islandora_solr_facet_escape($original_value),
            ));
            return l($use_value, 'islandora/search', array(
              "query" => array(
                "type" => "dismax",
                "f" => array(
                  $solr_query,
                ),
              ),
            ));
          }
          return $value;
        };

        // Want the values in array form even if it's a single valued field.
        if (!is_array($value)) {
          $value = (array) $value;
        }

        $solr_fields[$solr_field]['value'] = array_map($get_display_value, $value);
      }

      if (variable_get('islandora_solr_metadata_omit_empty_values', FALSE)) {
        $filter_function = function ($field_info) {
          return !empty($field_info['value']);
        };
        $filtered = array_filter($solr_fields, $filter_function);
        $solr_fields = $filtered;
      }

      if (variable_get('islandora_solr_metadata_dedup_values', FALSE)) {
        foreach ($solr_fields as &$field) {
          $field['value'] = array_unique($field['value']);
        }
        unset($field);
      }

      // After values are omitted/dedupped filter down the displayed results.
      foreach ($solr_fields as &$field) {
        // Check if the field is configured to filter based on whole field.
        if (($truncation = drupal_array_get_nested_value($field, array('truncation'))) && $truncation['max_length'] > 0 && isset($truncation['truncation_type']) && $truncation['truncation_type'] == 'whole_field_option') {
          // Keep the updated value stored in an array.
          $updated_value[] = islandora_solr_truncate_field_display($field['value'], $truncation['max_length'], $truncation['ellipsis'], $truncation['word_safe'], $truncation['min_wordsafe_length'], "\n");
          // Replace value with updated field display.
          $field['value'] = $updated_value;
        }
        unset($field);
      }

      uasort($solr_fields, 'drupal_sort_weight');
      return TRUE;
    }
    else {
      return FALSE;
    }
  }
}


/**
 * Helper function to derive the site value from the islandora_solr_base_filter value.  Does not handle more than
 * one site value in the base filter.
 */
function _get_site_from_config() {
  $base_filter = variable_get('islandora_solr_base_filter');
  // look for the value for RELS_EXT_isMemberOfSite_uri_ms
  $and_filters = explode(" AND ", $base_filter);
  $final_filters = array();
  foreach ($and_filters as $exploded) {
    $nl_exploded = explode("\r\n", $exploded);
    if (is_array($nl_exploded)) {
      foreach ($nl_exploded as $nl_element) {
        $final_filters[] = trim($nl_element);
      }
    }
    else {
      $final_filters[] = $nl_exploded;
    }
  }
  $found_filter = '';
  foreach ($final_filters as $filter) {
    if (strstr($filter, 'RELS_EXT_isMemberOfSite_uri_ms') <> '') {
      $found_filter = $filter;
    }
  }
  if ($found_filter) {
    $site = stripslashes(str_replace(array('"', 'RELS_EXT_isMemberOfSite_uri_ms:'), '', $found_filter));
    return $site;
  }
}
