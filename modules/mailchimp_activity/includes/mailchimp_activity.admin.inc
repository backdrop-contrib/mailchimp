<?php
/**
 * @file
 * Administration pages for mailchimp_activity module.
 */

/**
 * Returns a form for a mailchimp_activity_entity.
 */
function mailchimp_activity_entity_form($form, &$form_state, MailchimpActivityEntity $mailchimp_activity_entity = NULL, $op = 'edit') {
  if ($op == 'clone') {
    $mailchimp_activity_entity->label .= ' (cloned)';
    $mailchimp_activity_entity->name = '';
  }
  $entitynotnull = isset($mailchimp_activity_entity->mailchimp_activity_entity_id);
  $form_state['mailchimp_activity'] = $mailchimp_activity_entity;

  $form['label'] = array(
    '#title' => t('Label'),
    '#type' => 'textfield',
    '#default_value' => $entitynotnull ? $mailchimp_activity_entity->label : '',
    '#description' => t('The human-readable name of this activity entity.'),
    '#required' => TRUE,
    '#weight' => -10,
  );

  $form['name'] = array(
    '#title' => t('Name'),
    '#type' => 'machine_name',
    '#default_value' => $entitynotnull ? $mailchimp_activity_entity->name : '',
    '#description' => t('machine name should only contain lowercase letters & underscores'),
    '#disabled' => !empty($mailchimp_activity_entity->name),
    '#required' => TRUE,
    '#machine_name' => array(
      'exists' => 'mailchimp_activity_load_entities',
      'source' => array('label'),
    ),
    '#weight' => -9,
  );

  $form['backdrop_entity'] = array(
    '#title' => t('Backdrop entity'),
    '#type' => 'fieldset',
    '#attributes' => array(
      'id' => array('mailchimp-activity-backdrop-entity'),
    ),
    '#weight' => -8,
  );
  // Prep the entity type list before creating the form item:
  $entity_types = array('' => t('-- Select --'));
  foreach (entity_get_info() as $entity_type => $info) {
    // Ignore MailChimp entity types:
    if (strpos($entity_type, 'mailchimp') === FALSE) {
      $entity_types[$entity_type] = $info['label'];
    }
  }
  asort($entity_types);
  $form['backdrop_entity']['entity_type'] = array(
    '#title' => t('Entity type'),
    '#type' => 'select',
    '#options' => $entity_types,
    '#default_value' => $entitynotnull ? $mailchimp_activity_entity->entity_type : 0,
    '#required' => TRUE,
    '#description' => t('Select an entity type to enable mailchimp history on.'),
    '#ajax' => array(
      'callback' => 'mailchimp_activity_mapping_form_callback',
      'wrapper' => 'mailchimp-activity-backdrop-entity',
    ),
    '#weight' => -8,
  );

  $form_entity_type = & $form_state['values']['entity_type'];
  if (!$form_entity_type && $entitynotnull) {
    $form_entity_type = $mailchimp_activity_entity->entity_type;
  }
  if ($form_entity_type) {
    // Prep the bundle list before creating the form item:
    $bundles = array('' => t('-- Select --'));
    $info  = entity_get_info($form_entity_type);
    foreach ($info['bundles'] as $key => $bundle) {
      $bundles[$key] = $bundle['label'];
    }
    asort($bundles);
    $form['backdrop_entity']['bundle'] = array(
      '#title' => t('Entity bundle'),
      '#type' => 'select',
      '#required' => TRUE,
      '#description' => t('Select a Backdrop entity bundle with an email address to report on.'),
      '#options' => $bundles,
      '#default_value' => $entitynotnull ? $mailchimp_activity_entity->bundle : 0,
      '#weight' => -7,
      '#ajax' => array(
        'callback' => 'mailchimp_activity_mapping_form_callback',
        'wrapper' => 'mailchimp-activity-backdrop-entity',
      ),
    );

    $form_bundle = & $form_state['values']['bundle'];
    if (!$form_bundle && $entitynotnull) {
      $form_bundle = $mailchimp_activity_entity->bundle;
    }
    if ($form_bundle) {
      // Prep the field & properties list before creating the form item:
      $fields = mailchimp_activity_email_fieldmap_options($form_entity_type, $form_bundle);
      $form['backdrop_entity']['email_property'] = array(
        '#title' => t('Email Property'),
        '#type' => 'select',
        '#required' => TRUE,
        '#description' => t('Select the field which contains the email address'),
        '#options' => $fields,
        '#default_value' => $entitynotnull ? $mailchimp_activity_entity->email_property : 0,
        '#maxlength' => 127,
        '#weight' => -6,
      );
    }
  }
  $form['enabled'] = array(
    '#title' => t('Enabled'),
    '#type' => 'checkbox',
    '#default_value' => $entitynotnull ? $mailchimp_activity_entity->enabled : TRUE,
    '#weight' => -5,
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#value' => t('Save Entity'),
    '#type' => 'submit',
  );

  return $form;
}

/**
 * Ajax callback for mailchimp_activity_mapping_form().
 */
function mailchimp_activity_mapping_form_callback($form, &$form_state) {
  return $form['backdrop_entity'];
}

/**
 * Validation callback for mailchimp_activity_entity_form().
 */
function mailchimp_activity_entity_form_validate($form, &$form_state) {
  if ($form_state['submitted']) {
    $extant_mc_entities = entity_load('mailchimp_activity_entity');
    $form_id = $form_state['mailchimp_activity_entity']->mailchimp_activity_entity_id;
    $form_bundle = $form_state['values']['bundle'];
    $form_entity_id = $form_state['values']['entity_type'];
    foreach ($extant_mc_entities as $extant_ent) {
      if ($form_bundle == $extant_ent->bundle &&
          $form_entity_id == $extant_ent->entity_type &&
          $form_id != $extant_ent->mailchimp_activity_entity_id) {
        form_set_error(
          'bundle',
          t('A MailChimp Activity Entity already exists for this Bundle. Either select a different Bundle or edit the !link for this bundle.',
            array(
              '!link' => l(t('existing MailChimp Activity Entity'), "admin/config/services/mailchimp/activity/manage/{$extant_ent->name}"),
            )
          ));
      }
    }
  }
}

/**
 * Submit handler for mailchimp_activity_entity_form().
 */
function mailchimp_activity_entity_form_submit($form, &$form_state) {
  $dummy_entity = entity_create($form_state['values']['entity_type'], array('type' => $form_state['values']['bundle']));
  $entity_info = entity_get_info($dummy_entity->type);
  if (!empty($entity_info)) {
    // Create empty entity ID field so entity_uri doesn't result in an error.
    $entity_id_field = $entity_info['entity keys']['id'];
    $dummy_entity->$entity_id_field = NULL;
  }
  $uri = entity_uri($form_state['values']['entity_type'], $dummy_entity);
  $values = $form_state['values'];
  $values['entity_path'] = $uri['path'];
  if ($form_state['op'] == 'add' || $form_state['op'] == 'clone') {
    $activity_entity = entity_create('mailchimp_activity_entity', $values);
  }
  else {
    $activity_entity = $form_state['mailchimp_activity_entity'];
    foreach ($values as $key => $val) {
      $activity_entity->{$key} = $val;
    }
  }
  $activity_entity->save();
  // Make sure the new items appear in the menus:
  menu_rebuild();
  $form_state['redirect'] = 'admin/config/services/mailchimp/activity';
}

/**
 * Return all possible Backdrop properties for a given entity type.
 *
 * @param string $entity_type
 *   Name of entity whose properties to list.
 * @param string $entity_bundle
 *   Optional entity bundle to get properties for.
 *
 * @return array
 *   List of entities that can be used as an #options list.
 */
function mailchimp_activity_email_fieldmap_options($entity_type, $entity_bundle = NULL) {
  $options = array('' => t('-- Select --'));

  $properties = entity_plus_get_all_property_info($entity_type);
  if (isset($entity_bundle)) {
    $info = entity_plus_get_property_info($entity_type);
    $properties = $info['properties'];
    if (isset($info['bundles'][$entity_bundle])) {
      $properties += $info['bundles'][$entity_bundle]['properties'];
    }
  }

  foreach ($properties as $key => $property) {
    $type = isset($property['type']) ? entity_plus_property_extract_innermost_type($property['type']) : 'text';
    $is_entity = ($type == 'entity') || (bool) entity_get_info($type);
    // Leave entities out of this.
    if (!$is_entity) {
      if (isset($property['field']) && $property['field'] && !empty($property['property info'])) {
        foreach ($property['property info'] as $sub_key => $sub_prop) {
          $options[$property['label']][$key . ':' . $sub_key] = $sub_prop['label'];
        }
      }
      else {
        $options[$key] = $property['label'];
      }
    }
  }

  return $options;
}
