<?php
/**
 * @file
 * Administration pages for mailchimp_campaign module.
 */

/**
 * Returns a form for creating a campaign.
 */
function mailchimp_campaign_campaign_form($form, &$form_state, MailChimpCampaign $campaign = NULL) {
  $path = drupal_get_path('module', 'mailchimp_campaign');
  drupal_add_js($path . '/js/mailchimp_campaign.utils.js', 'file');
  drupal_add_css($path . '/mailchimp_campaign.css', 'file');

  $form_state['campaign'] = $campaign;
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#description' => t('An internal name to use for this campaign. By default, the campaign subject will be used.'),
    '#required' => FALSE,
    '#default_value' => ($campaign) ? $campaign->mc_data['title'] : '',
  );
  $form['subject'] = array(
    '#type' => 'textfield',
    '#title' => t('Subject'),
    '#required' => TRUE,
    '#default_value' => ($campaign) ? $campaign->mc_data['subject'] : '',
  );
  $form['list_id'] = array(
    '#type' => 'select',
    '#title' => t('List'),
    '#description' => t('Select the list this campaign should be sent to.'),
    '#options' => _mailchimp_campaign_build_option_list(mailchimp_get_lists()),
    '#default_value' => ($campaign) ? $campaign->mc_data['list_id'] : -1,
    '#required' => TRUE,
    '#ajax' => array(
      'callback' => 'mailchimp_campaign_list_segment_callback',
      'method' => 'replace',
      'wrapper' => 'list-segments-wrapper',
    ),
  );

  if (!empty($form_state['values']['list_id'])) {
    $list_id = $form_state['values']['list_id'];
  }
  elseif ($campaign && $campaign->mc_data) {
    $list_id = $campaign->mc_data['list_id'];
    if (isset($campaign->mc_data['saved_segment']['id'])) {
      $segment_id = $campaign->mc_data['saved_segment']['id'];
    }
  }

  $list_segments = array();
  if (isset($list_id)) {
    $list_segments = mailchimp_campaign_get_list_segments($list_id, 'saved');
  }

  if (!empty($list_segments)) {
    $form['list_segment_id'] = array(
      '#type' => 'select',
      '#title' => t('List Segment'),
      '#description' => t('Select the list segment this campaign should be sent to.'),
      '#options' => _mailchimp_campaign_build_option_list($list_segments, '-- Entire list --'),
      '#default_value' => (isset($segment_id)) ? $segment_id : '',
    );
  }
  else {
    $form['list_segment_id'] = array();
  }
  $form['list_segment_id']['#prefix'] = '<div id="list-segments-wrapper">';
  $form['list_segment_id']['#suffix'] = '</div>';

  $form['from_email'] = array(
    '#type' => 'textfield',
    '#title' => t('From Email'),
    '#description' => t('the From: email address for your campaign message.'),
    '#default_value' => ($campaign) ? $campaign->mc_data['from_email'] : variable_get('site_mail'),
    '#size' => 40,
    '#maxlength' => 255,
    '#required' => TRUE,
  );
  $form['from_name'] = array(
    '#type' => 'textfield',
    '#title' => t('From Name'),
    '#description' => t('the From: name for your campaign message (not an email address)'),
    '#default_value' => ($campaign) ? $campaign->mc_data['from_name'] : variable_get('site_name'),
    '#size' => 40,
    '#maxlength' => 255,
  );
  $template_type_labels = array(
    'user' => 'My Custom Templates',
    'basic' => 'Basic Templates',
    'gallery' => 'Themes',
  );
  $form['template_id'] = array(
    '#type' => 'select',
    '#title' => t('Template'),
    '#description' => t('Select a MailChimp user template to use. Due to a limitation in the API, only templates that do not contain repeating sections are available. If empty, the default template will be applied.'),
    '#options' => _mailchimp_campaign_build_option_list(mailchimp_campaign_get_templates(), '-- Select --', $template_type_labels),
    '#default_value' => ($campaign) ? $campaign->mc_data['template_id'] : -1,
    '#ajax' => array(
      'callback' => 'mailchimp_campaign_template_callback',
      'wrapper' => 'content-sections',
    ),
  );
  $form['content'] = array(
    '#id' => 'content-sections',
    '#type' => 'fieldset',
    '#title' => t('Content sections'),
    '#description' => t('The HTML content or, if a template is selected, the content for each section.'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
    '#tree' => TRUE,
  );

  $mc_template = NULL;
  if (!empty($form_state['values']['template_id'])) {
    $mc_template = mailchimp_campaign_get_templates($form_state['values']['template_id']);
  }
  else {
    if (($campaign) && $campaign->mc_template) {
      $mc_template = $campaign->mc_template;
    }
  }

  if (isset($list_id)) {
    $merge_vars = mailchimp_get_mergevars(array($list_id));
    if (isset($merge_vars[$list_id])) {
      $form['content']['merge_vars'] = array(
        '#type' => 'fieldset',
        '#title' => 'Merge Tags',
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
        '#children' => _mailchimp_campaign_build_merge_vars_html(
          $merge_vars[$list_id]['merge_vars']),
      );
    }
  }

  $form['content']['entity_import'] = array(
    '#id' => 'entity-import',
    '#type' => 'fieldset',
    '#title' => t('Site content import'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $entity_info = _mailchimp_campaign_get_entities_for_content_import();
  $entity_options = _mailchimp_campaign_build_entity_option_list($entity_info);

  if (!empty($form_state['values']['content']['entity_import']['entity_type'])) {
    $entity_type = $form_state['values']['content']['entity_import']['entity_type'];
  }
  else {
    reset($entity_options);
    $entity_type = current(array_keys($entity_options));
  }

  $entity_view_mode_options = _mailchimp_campaign_build_entity_view_mode_option_list($entity_info[$entity_type]);

  $form['content']['entity_import']['entity_type'] = array(
    '#type' => 'select',
    '#title' => t('Entity Type'),
    '#options' => $entity_options,
    '#default_value' => $entity_type,
    '#ajax' => array(
      'callback' => 'mailchimp_campaign_entity_type_callback',
      'method' => 'replace',
      'wrapper' => 'content-entity-lookup-wrapper',
    ),
  );
  $form['content']['entity_import']['entity_type']['#attributes']['class'][] = 'entity-import-entity-type';

  $form['content']['entity_import']['entity_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Entity Title'),
    // Pass entity type as first parameter to entity autocomplete callback.
    '#autocomplete_path' => 'admin/config/services/mailchimp/campaigns/entities/' . $entity_type,
    '#prefix' => '<div id="content-entity-lookup-wrapper">',
  );
  $form['content']['entity_import']['entity_id']['#attributes']['class'][] = 'entity-import-entity-id';

  $form['content']['entity_import']['entity_view_mode'] = array(
    '#type' => 'select',
    '#title' => t('View Mode'),
    '#options' => $entity_view_mode_options,
    '#suffix' => '</div>',
  );
  $form['content']['entity_import']['entity_view_mode']['#attributes']['class'][] = 'entity-import-entity-view-mode';

  $form['content']['entity_import']['entity_import_link'] = array(
    '#type' => 'item',
    '#markup' => '<a id="add-entity-token" href="javascript:void(0);">Add entity token</a>',
  );

  $form['content']['entity_import']['entity_import_tag'] = array(
    '#type' => 'item',
    '#markup' => '<div id="entity-import-tag-field"></div>',
  );

  if ($mc_template) {
    if (strpos($mc_template['info']['source'], 'mc:repeatable')) {
      drupal_set_message(t('WARNING: This template section has repeating sections, which are not supported. You may want to select a different template.'), 'warning');
    }
    foreach ($mc_template['info']['default_content'] as $section => $content) {
      // Set the default value and text format to either saved campaign values
      // or defaults coming from the MailChimp template.
      $default_value = $content;
      $format = 'mailchimp_campaign';
      if ($campaign && $campaign->template[$section]) {
        $default_value = $campaign->template[$section]['value'];
        $format = $campaign->template[$section]['format'];
      }
      $form['content'][$section . '_wrapper'] = array(
        '#type' => 'fieldset',
        '#title' => check_plain(drupal_ucfirst($section)),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
      );
      $form['content'][$section . '_wrapper'][$section] = array(
        '#type' => 'text_format',
        '#format' => $format,
        '#title' => check_plain(drupal_ucfirst($section)),
        '#default_value' => $default_value,
      );
    }
  }
  else {
    $form['content']['html_wrapper'] = array(
      '#type' => 'fieldset',
      '#title' => t('Content'),
      '#collapsible' => TRUE,
      '#collapsed' => TRUE,
    );
    $form['content']['html_wrapper']['html'] = array(
      '#type' => 'text_format',
      '#format' => ($campaign) ? $campaign->template['html']['format'] : 'mailchimp_campaign',
      '#title' => t('Content'),
      '#description' => t('The HTML content of the campaign.'),
      '#access' => empty($form_state['values']['template_id']),
      '#default_value' => ($campaign) ? $campaign->template['html']['value'] : '',
    );
  }

  // Message preview:
  if (isset($form_state['mailchimp_campaign_campaign_preview'])) {
    $form['preview_wrapper'] = array(
      '#title' => t('Campaign content preview'),
      '#type' => 'fieldset',
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    $form['preview_wrapper']['preview'] = array(
      '#markup' => $form_state['mailchimp_campaign_campaign_preview'],
    );
  }

  $form['actions']['preview'] = array(
    '#type' => 'submit',
    '#value' => t('Preview content'),
    '#weight' => 10,
    '#submit' => array('mailchimp_campaign_campaign_preview'),
  );
  $form['actions']['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save as draft'),
  );

  return $form;
}

/**
 * AJAX callback when changing template ID.
 */
function mailchimp_campaign_template_callback($form, $form_state) {
  return $form['content'];
}

/**
 * AJAX callback when changing list ID.
 */
function mailchimp_campaign_list_segment_callback($form, $form_state) {
  return $form['list_segment_id'];
}

/**
 * AJAX callback when changing entity type.
 *
 * @return array
 *   Entity form elements array.
 */
function mailchimp_campaign_entity_type_callback($form, $form_state) {
  return array(
    $form['content']['entity_import']['entity_id'],
    $form['content']['entity_import']['entity_view_mode'],
  );
}

/**
 * Preview callback for mailchimp_campaign_campaign_form().
 */
function mailchimp_campaign_campaign_preview($form, &$form_state) {
  $text = '';
  $content = mailchimp_campaign_render_template($form_state['values']['content']);
  foreach ($content as $key => $section) {
    $text .= "<h3>$key</h3>" . $section;
  }

  $form_state['mailchimp_campaign_campaign_preview'] = $text;
  $form_state['rebuild'] = TRUE;
}

/**
 * Validation callback for mailchimp_campaign_campaign_form().
 */
function mailchimp_campaign_campaign_form_validate($form, &$form_state) {
  if (!valid_email_address($form_state['values']['from_email'])) {
    form_set_error('from_email', t('Please provide a valid From email address.'));
  }
}

/**
 * Submit handler for mailchimp_campaign_campaign_form().
 */
function mailchimp_campaign_campaign_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $options = array(
    'title' => $values['title'],
    'subject' => $values['subject'],
    'list_id' => $values['list_id'],
    'from_email' => $values['from_email'],
    'from_name' => check_plain($values['from_name']),
    'template_id' => $values['template_id'],
  );
  $segment_options = NULL;
  if (isset($values['list_segment_id']) && !empty($values['list_segment_id'])) {
    $segment_options = array(
      'saved_segment_id' => $values['list_segment_id'],
    );
  }

  // Parse template content to remove wrapper elements from tree.
  $template_content = array();
  $content_keys = array_keys($values['content']);
  foreach ($content_keys as $content_key) {
    if (strpos($content_key, '_wrapper') !== FALSE) {
      // If this element is a wrapper, add the element contained
      // within the wrapper to the template content.
      $new_content_key = str_replace('_wrapper', '', $content_key);
      $template_content[$new_content_key] = $values['content'][$content_key][$new_content_key];
    }
    else {
      // If this element is not a wrapper, add it to the template content.
      $template_content[$content_key] = $values['content'][$content_key];
    }
  }

  $campaign_id = (isset($form_state['campaign']) ? $form_state['campaign']->mc_campaign_id : NULL);
  mailchimp_campaign_save_campaign($template_content, $options, $segment_options, $campaign_id);

  cache_clear_all('mailchimp_campaign_campaigns', 'cache');

  $form_state['redirect'][] = 'admin/config/services/mailchimp/campaigns';
}

/**
 * Returns an array of entities based on data from entity_get_info().
 *
 * Filters out entities that do not contain a title field, as they cannot
 * be used to import content into templates.
 *
 * @return array
 *   Filtered entities from entity_get_info().
 */
function _mailchimp_campaign_get_entities_for_content_import() {
  $entity_info = entity_get_info();

  $filtered_entities = array();
  foreach ($entity_info as $key => $entity) {
    if (in_array('title', $entity['schema_fields_sql']['base table'])) {
      $filtered_entities[$key] = $entity;
    }
  }

  return $filtered_entities;
}

/**
 * Returns an options list of entities based on data from entity_get_info().
 *
 * Filters out entities that do not contain a title field, as they cannot
 * be used to import content into templates.
 *
 * @param array $entity_info
 *   Array of entities as returned by entity_get_info().
 *
 * @return array
 *   Associative array of entity IDs to name.
 */
function _mailchimp_campaign_build_entity_option_list($entity_info) {
  $options = array();
  foreach ($entity_info as $entity_id => $entity_data) {
    $options[$entity_id] = $entity_data['label'];
  }

  return $options;
}

/**
 * Returns an options list of entity view modes.
 *
 * @param array $entity
 *   Array of entity data as returned by entity_get_info().
 *
 * @return array
 *   Associative array of view mode IDs to name.
 */
function _mailchimp_campaign_build_entity_view_mode_option_list(array $entity) {
  $options = array();
  foreach ($entity['view modes'] as $view_mode_id => $view_mode_data) {
    $options[$view_mode_id] = $view_mode_data['label'];
  }

  return $options;
}

/**
 * Returns an options list for a given array of items.
 *
 * @param array $list
 *   Array of item data containing 'id' and 'name' properties.
 * @param string $no_selection_label
 *   The option value to display when no option is selected.
 *
 * @return array
 *   Associative array of item IDs to name.
 */
function _mailchimp_campaign_build_option_list($list, $no_selection_label = '-- Select --', $labels = array()) {
  $options = array();
  if ($no_selection_label) {
    $options[''] = $no_selection_label;
  }
  foreach ($list as $index => $item) {
    if (!isset($item['id'])) {
      $label = isset($labels[$index]) ? $labels[$index] : $index;
      if (count($item)) {
        $options[$label] = _mailchimp_campaign_build_option_list($item, FALSE, $labels);
      }
    }
    else {
      $options[$item['id']] = $item['name'];
    }
  }

  return $options;
}

/**
 * Builds a HTML string used to render merge vars on the campaign form.
 *
 * @param array $merge_vars
 *   Array of merge vars. @see mailchimp_lists_get_merge_vars
 *
 * @return string
 *   HTML string containing formatted merge vars.
 */
function _mailchimp_campaign_build_merge_vars_html($merge_vars) {
  $html = '';

  if (!empty($merge_vars)) {
    $html .= '<table>';
    foreach ($merge_vars as $var) {
      $html .= '<tr>'
        . '<td>' . $var['name'] . '</td>'
        . '<td><a id="merge-var-'
        . $var['tag'] . '" class="add-merge-var" href="javascript:void(0);">*|' . $var['tag'] . '|*</a></td>'
        . '</tr>';
    }
    $html .= '</table>';
  }
  else {
    $html .= t('No custom merge vars exist for the current list.');
  }

  $html .= '<p>' . l(t('Standard MailChimp merge tags are available.'),
      'http://kb.mailchimp.com/article/all-the-merge-tags-cheatsheet',
      array('attributes' => array('target' => '_blank'))) . '</p>';

  return $html;
}

/**
 * List deletion form.
 */
function mailchimp_campaign_delete_form($form, &$form_state, MailChimpCampaign $campaign) {
  $form_state['campaign'] = $campaign;
  return confirm_form($form,
    t('Are you sure you want to delete %name?', array('%name' => $campaign->label())),
    'admin/config/services/mailchimp/campaigns/' . $campaign->mc_campaign_id,
    t('This action will delete both the MailChimp campaign and Drupal entity and cannot be undone.'),
    t('Delete campaign')
  );
}

/**
 * Submit handler for mailchimp_campaign_delete_list_form().
 */
function mailchimp_campaign_delete_form_submit($form, &$form_state) {
  $campaign = $form_state['campaign'];
  if (mailchimp_campaign_delete_campaign($campaign->mc_campaign_id)) {
    drupal_set_message(t('Campaign %name has been deleted.',
      array('%name' => $campaign->label()))
    );
  }
  drupal_goto('admin/config/services/mailchimp/campaigns');
}

/**
 * List deletion form.
 */
function mailchimp_campaign_send_form($form, &$form_state, MailChimpCampaign $campaign) {
  $form_state['campaign'] = $campaign;
  return confirm_form($form,
    t('Are you sure you want to send %name?', array('%name' => $campaign->label())),
    'admin/config/services/mailchimp/campaigns/' . $campaign->mc_campaign_id,
    t('This action will send the campaign through MailChimp and cannot be undone.'),
    t('Send campaign')
  );
}

/**
 * Submit handler for mailchimp_campaign_delete_list_form().
 */
function mailchimp_campaign_send_form_submit($form, &$form_state) {
  $campaign = $form_state['campaign'];
  mailchimp_campaign_send_campaign($campaign);
  drupal_set_message(t('Campaign %name has been sent.',
      array('%name' => $campaign->label()))
  );
  drupal_goto('admin/config/services/mailchimp/campaigns');
}