<?php

/**
 * @file
 * This file contains utility functions
 */

/**
 * Returns a single solr field value for an object.
 *
 * @param object $object
 *   Islandora Object
 * @param string $solr_field
 *   the name of the solr field
 * @return string
 *   The solr_field value
 */
function islandora_solr_metadata_solr_value($object, $solr_field) {
  module_load_include('inc', 'islandora_solr', 'includes/utilities');
  $ret_val = '';
  if (!empty($solr_field)) {
    $query_processor = new IslandoraSolrQueryProcessor();
    $query_processor->solrQuery = 'PID:"' . $object->id . '"';
    $query_processor->solrParams['fl'] = $solr_field;
    $query_processor->executeQuery(FALSE);
    if ($query_processor->islandoraSolrResult['response']['numFound'] > 0) {
      $solr_results_doc = $query_processor->islandoraSolrResult['response']['objects']['0']['solr_doc'];
      // Ensure we only provide results for what is specified.
      $ret_val = implode(", ", (array)$solr_results_doc[$solr_field]);
    }
  }
  return $ret_val;
}
