<?php
/**
 * @file
 * Provides infrequently used functions for polldaddy.
 */

require_once('polldaddy.api.php');

/**
 * Menu callback: display the block addition form.
 */
function polldaddy_add_block_form($form, &$form_state) {
  module_load_include('inc', 'block', 'block.admin');
  return block_admin_configure($form, $form_state, 'polldaddy', NULL);
}

/**
 * Save the new menu block.
 */
function polldaddy_add_block_form_submit($form, &$form_state) {
  // Determine the delta of the new block.
  $block_ids = variable_get('polldaddy_ids', array());
  $delta = empty($block_ids) ? 1 : max($block_ids) + 1;

  // Save the new array of blocks IDs.
  $block_ids[] = $delta;
  variable_set('polldaddy_ids', $block_ids);

  // Save the block configuration.
  polldaddy_block_save($delta, $form_state['values']);

  // Run the normal new block submission (borrowed from block_add_block_form_submit).
  // Store block delta to allow other modules to work with new block.
  $form_state['values']['delta'] = $delta;

  $query = db_insert('block')->fields(array('visibility', 'pages', 'custom', 'title', 'module', 'theme', 'status', 'weight', 'delta', 'cache'));
  foreach (list_themes() as $key => $theme) {
    if ($theme->status) {
      $query->values(array(
        'visibility' => (int) $form_state['values']['visibility'],
        'pages' => trim($form_state['values']['pages']),
        'custom' => (int) $form_state['values']['custom'],
        'title' => $form_state['values']['title'],
        'module' => $form_state['values']['module'],
        'theme' => $theme->name,
        'status' => 0,
        'weight' => 0,
        'delta' => $delta,
        'cache' => DRUPAL_NO_CACHE,
      ));
    }
  }
  $query->execute();

  $query = db_insert('block_role')->fields(array('rid', 'module', 'delta'));
  foreach (array_filter($form_state['values']['roles']) as $rid) {
    $query->values(array(
      'rid' => $rid,
      'module' => $form_state['values']['module'],
      'delta' => $delta,
    ));
  }
  $query->execute();

  // Store regions per theme for this block
  foreach ($form_state['values']['regions'] as $theme => $region) {
    db_merge('block')->key(array('theme' => $theme, 'delta' => $delta, 'module' => $form_state['values']['module']))->fields(array(
      'region' => ($region == BLOCK_REGION_NONE ? '' : $region),
      'pages' => trim($form_state['values']['pages']),
      'status' => (int) ($region != BLOCK_REGION_NONE),
    ))->execute();
  }

  drupal_set_message(t('The block has been created.'));
  cache_clear_all();
  $form_state['redirect'] = 'admin/structure/block';
  return;
}

/**
 * Alters the block admin form to add delete links next to menu blocks.
 */
function _polldaddy_form_block_admin_display_form_alter(&$form, $form_state) {
  foreach (variable_get('polldaddy_ids', array()) AS $delta) {
    $form['polldaddy_' . $delta]['delete'] = array('#value' => l(t('delete'), 'admin/build/block/delete-polldaddy/'. $delta));
  }
}

/**
 * Menu callback: confirm deletion of menu blocks.
 */
function polldaddy_delete(&$form_state, $delta = 0) {
  $title = _polldaddy_format_title(polldaddy_get_config($delta));
  $form['block_title'] = array('#type' => 'hidden', '#value' => $title);
  $form['delta'] = array('#type' => 'hidden', '#value' => $delta);

  return confirm_form($form, t('Are you sure you want to delete the "%name" block?', array('%name' => $title)), 'admin/build/block', NULL, t('Delete'), t('Cancel'));
}

/**
 * Deletion of menu blocks.
 */

    /*'polldaddy_use_style' => array(
      '#type' => 'checkbox',
      '#title' => 'Use polldaddy.com style. If checked the predefined style will not be loaded',
      '#default_value' => FALSE
    ),*/
function polldaddy_delete_submit($form, &$form_state) {
  // Remove the menu block configuration variables.
  $delta = $form_state['values']['delta'];
  $block_ids = variable_get('polldaddy_ids', array());
  unset($block_ids[array_search($delta, $block_ids)]);
  sort($block_ids);
  variable_set('polldaddy_ids', $block_ids);
  variable_del("polldaddy_{$delta}_poll_id");
  variable_del("polldaddy_{$delta}_use_style");
  db_query("DELETE FROM {blocks} WHERE module = 'polldaddy' AND delta = %d", $delta);
  db_query("DELETE FROM {blocks_roles} WHERE module = 'polldaddy' AND delta = %d", $delta);
  drupal_set_message(t('The "%name" block has been removed.', array('%name' => $form_state['values']['block_title'])));
  cache_clear_all();
  $form_state['redirect'] = 'admin/build/block';
  return;
}

/**
 * Returns the 'list' $op info for hook_block().
 */
function _polldaddy_block_list() {
  $blocks = array();
  foreach (variable_get('polldaddy_ids', array()) AS $delta) {
    $blocks[$delta]['info'] = _polldaddy_format_title(polldaddy_get_config($delta));
    // Menu blocks can't be cached because each menu item can have
    // a custom access callback. menu.inc manages its own caching.
    $blocks[$delta]['cache'] = BLOCK_NO_CACHE;
  }
  return $blocks;
}

/**
 * Returns the configuration form for a menu tree.
 *
 * @param $form_state
 *   array An associated array of configuration options should be present in the
 *   'values' key. If none are given, default configuration is assumed.
 * @return
 *   array The form in Form API format.
 */
function polldaddy_configure_form(&$form_state) {
  $config = array();
  // Get the config from the form state.
  if (!empty($form_state['values'])) {
    $config = $form_state['values'];
  }
  // Merge in the default configuration.
  $config += polldaddy_get_config();

  $form['polldaddy-wrapper-start'] = array(
    '#value' => '<div id="polldaddy-settings" class="polldaddy-configure-form">',
  );
  $polls = polldaddy_get_polls();

  $form['poll_id'] = array(
    '#type' => 'select',
    '#default_value' => $config['poll_id']?$config['poll_id']:'',
    '#title' => t('Poll'),
    '#options' => $polls,
    '#description' => t('Select the poll you want to put in a block'),
  );

  $form['use_style'] = array(
    '#type' => 'checkbox',
    '#default_value' => $config['use_style']?$config['use_style']:FALSE,
    '#title' => t('Use style defined at <a href="http://wwww.polldaddy.com" target="_blank">http://www.polldaddy.com</a>'),
    '#description' => t('If checked the default style defined at polldaddy.com will be used'),
  );


  $form['polldaddy-wrapper-close'] = array('#value' => '</div>');
  return $form;
}

/**
 * Polldaddy admin settings form.
 *
 * @return
 *   The settings form used by Menu block.
 */
function polldaddy_admin_settings_form($form_state) {
  $settings = variable_get('polldaddy_settings', array('polldaddy_partner_guid' => '',
                                                       'polldaddy_usercode' => ''));
  //drupal_set_message(print_r($settings,1));
  $form['polldaddy_settings'] = array (
    'polldaddy_partner_guid' => array (
      '#type' => 'textfield',
      '#title' => 'Partner GUID',
      '#default_value' => $settings['polldaddy_partner_guid']?$settings['polldaddy_partner_guid']:''
    ),
    'polldaddy_style_id' => array (
      '#type' => 'textfield',
      '#title' => 'Style Id',
      '#default_value' => $settings['polldaddy_style_id']?$settings['polldaddy_style_id']:''
    ),
  );
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );
  //drupal_set_message($settings['polldaddy_usercode']);
  return $form;
}

/**
 * Form submission handler.
 */
function polldaddy_admin_settings_form_submit($form, &$form_state) {
  $settings = array();
  $settings['polldaddy_partner_guid']  = $form_state['values']['polldaddy_partner_guid'];
  $settings['polldaddy_style_id']  = $form_state['values']['polldaddy_style_id'];
  $old_settings = variable_get('polldaddy_settings', array());
  $usercode = polldaddy_get_usercode($settings['polldaddy_partner_guid']);
  //drupal_set_message($usercode);
  $settings['polldaddy_usercode'] = $usercode;
  variable_set('polldaddy_settings', $settings);
  //polldaddy_get_poll('4920991');
}

function polldaddy_send_vote($poll_id, $answer_id){
  $useragent="Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US; rv:1.8.1.1) Gecko/20061204 Firefox/2.0.0.1";
  $req = 'http://polls.polldaddy.com/pollVoteFlash.aspx?%3Canswer%20poll_id=%22'.$poll_id.'%22%20pol_type=%220%22%20answer=%22'.$answer_id.'%22%20other=%22%22%20poll_other=%220%22%20vote=%221%22%20/%3E';
  $ch = curl_init();
  curl_setopt($ch, CURLOPT_URL, $req);
  curl_setopt($ch, CURLOPT_HEADER, true);
  curl_setopt($ch, CURLOPT_USERAGENT, $useragent);
  curl_setopt($ch, CURLOPT_VERBOSE, false);
  curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
  $result = curl_exec ($ch);
  curl_close($ch);
}
