<?php

/**
 * @file
 *   drush integration for apachesolr_users.
 */

/**
 * Implements hook_drush_command().
 *
 * @return array
 *   An associative array describing your command(s).
 */
function apachesolr_users_drush_command() {
  $items = array();

  $items['solr-users-delete-index'] = array(
    'callback' => 'apachesolr_users_drush_solr_delete_index',
    'description' => dt('Deletes the users from the index.'),
    'arguments' => array(),
    'examples' => array(),
  );
  $items['solr-users-index'] = array(
    'callback' => 'apachesolr_users_drush_solr_index',
    'description' => dt('Indexes all the users.'),
    'arguments' => array(),
    'examples' => array(),
  );
  return $items;
}

/**
 * Implements hook_drush_help().
 *
 * This function is called whenever a drush user calls
 * 'drush help <name-of-your-command>'
 *
 * @param string $section
 *   A string with the help section (prepend with 'drush:')
 *
 * @return string
 *   A string with the help text for your command.
 */
function apachesolr_users_drush_help($section) {
  switch ($section) {
    case 'drush:solr-users-delete-index':
      return dt("Used without parameters, this command deletes the entire user Solr index. After the index has been deleted, all users will be indexed again on future cron runs.");
    case 'drush:solr-users-index':
      return dt("Indexes all the users.");
  }
}

/**
 * Deletes all users from index
 */
function apachesolr_users_drush_solr_delete_index() {
  module_load_include('module', 'apachesolr_users', 'apachesolr_users');

  db_query("DELETE FROM {apachesolr_users_queue}");
  db_query("INSERT INTO {apachesolr_users_queue} (uid, status, modified) SELECT u.uid, u.status, u.created FROM {users} u WHERE uid <> 0");

  // Users created in the future get a modified time of now.
  db_query("UPDATE {apachesolr_users_queue} SET modified = %d WHERE modified > %d", $_SERVER['REQUEST_TIME'], $_SERVER['REQUEST_TIME']);

  apachesolr_users_clear_last_index();

  $users = db_query("SELECT uid FROM {apachesolr_users_queue} WHERE status = 1");

  while ($user_obj = db_fetch_object($users)) {
      apachesolr_users_delete_user_from_index($user_obj->uid);
  }
  
  drush_print(t('Deleted the user Solr index'));
}

/**
 * Indexes all the users
 */
function apachesolr_users_drush_solr_index() {
  module_load_include('module', 'apachesolr_users', 'apachesolr_users');
  $batch = array(
    'operations' => array(
      array('apachesolr_users_batch_index', array()),
    ),
    //'finished' => 'apachesolr_users_batch_index_finished',
    'title' => t('Indexing'),
    'init_message' => t('Preparing to submit users to Solr for indexing...'),
    'progress_message' => t('Submitting users to Solr...'),
    'error_message' => t('Solr Users indexing has encountered an error.'),
    'file' => drupal_get_path('module', 'apachesolr_users') . '/apachesolr_users.batch.inc',
  );
  batch_set($batch);
  drush_backend_batch_process();
}


