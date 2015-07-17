<?php

/**
 * @file
 * The Polldaddy Poll content type.
 */

/**
 * Callback function to supply a list of content types.
 */
$plugin = array(
  'title' => t('Polldaddy Poll'),
  'description' => t('Show a polldaddy poll.'),
  'category' => t('Miscellaneous'),
  'single' => TRUE,
  'render callback' => 'polldaddy_poll_render',
);

/**
 * Output function for polldaddy poll.
 */
function polldaddy_poll_render($subtype, $conf, $panel_args, $context) {
  $polldaddy_poll_id = $conf['polldaddy_poll_id'];
  $poll_id = ctools_context_keyword_substitute($polldaddy_poll_id, array(), array($context));

  $block = new stdClass();
  $block->content = polldaddy_poll_view($poll_id);
  return $block;
}

/**
 * Returns an edit form for the custom type.
 */
function polldaddy_polldaddy_poll_content_type_edit_form($form, &$form_state) {
  $conf = $form_state['conf'];

  // The form options for the footer
  $form['polldaddy_poll_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Poll id from polldaddy'),
    '#description' => t('Enter the id of the poll to pull from polldaddy.'),
    '#default_value' => $conf['polldaddy_poll_id'],
  );

  return $form;
}

/**
 * Submit function for the content type.
 */
function polldaddy_polldaddy_poll_content_type_edit_form_submit(&$form, &$form_state) {
  $form_state['conf']['polldaddy_poll_id'] = $form_state['values']['polldaddy_poll_id'];
}
