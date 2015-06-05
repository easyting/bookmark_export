<?php
/**
 * @file
 * Admin routine file.
 */

/**
 * Export backend controls form.
 */
function bookmark_export_admin_form($form, $form_state) {
  $form['notify'] = array(
    '#type' => 'checkbox',
    '#title' => t('Notify each user via email'),
    '#description' => t('Send an email for each user with a notification message.'),
  );

  $message = <<<MSG
Hello.
@library has gotten a new website.

Unfortunately, your bookmarks could not be transferred automatically to the new website, we have saved your bookmarks from the old website in a file that you can load on the new website.
The file can be downloaded here: @csv_url.

On the new website you can read more about how the file is imported.

Sincerely, @library.
MSG;
  $form['message'] = array(
    '#type' => 'textarea',
    '#title' => t('Notification template'),
    '#default_value' => $message,
    '#description' => t('Replacement tokens: @library, @csv_url'),
    '#states' => array(
      'visible' => array(
        ':input[name="notify"]' => array(
          'checked' => TRUE,
        ),
      ),
    ),
  );

  $form['source'] = array(
    '#type' => 'textfield',
    '#title' => t('CSV custom URL'),
    '#default_value' => '',
    '#description' => t('Use custom url for the resulting csv file <strong>(NO trailing slash)</strong>. Remember to setup virtual host accordingly.'),
    '#states' => array(
      'visible' => array(
        ':input[name="notify"]' => array(
          'checked' => TRUE,
        ),
      ),
    ),
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Start exporting'),
  );

  return $form;
}

/**
 * Custom validation handler for the export form.
 *
 * @see bookmark_export_admin_form()
 */
function bookmark_export_admin_form_validate($form, &$form_state) {
  $input = $form_state['input'];

  if (isset($input['notify']) && !empty($input['source']) && !valid_url($input['source'], TRUE)) {
    form_set_error('source', t('URL is malformed.'));
  }
}

/**
 * Custom submit handler for the export form.
 *
 * @see bookmark_export_admin_form()
 */
function bookmark_export_admin_form_submit($form, &$form_state) {
  $input = $form_state['input'];
  $notify = isset($input['notify']);
  $url_override = $input['source'];

  $user_bookmarks = bookmark_export_get_bookmarks(TRUE);
  $operations = array();
  foreach ($user_bookmarks as $k => &$v) {
    $csv_path = 'public://bookmark_exports/' . $k . '.csv';
    $csv_url = file_create_url($csv_path);

    if (!empty($url_override)) {
      $csv_url = $url_override . '/' . $k . '.csv';
    }

    if ($notify) {
      $t = array(
        '@library' => variable_get('site_name', ''),
        '@csv_url' => $csv_url,
      );
      $v['message'] = t($input['message'], $t);
      $v['notify'] = TRUE;
    }

    $operations[] = array('bookmark_export_batch_start', array($csv_path, $v));
  }

  $batch = array(
    'operations' => $operations,
    'finished' => 'bookmark_export_batch_finished',
    'title' => t('Exporting bookmarks...'),
    'init_message' => t('Preparing...'),
    'progress_message' => t('Users processed @current out of @total.'),
    'error_message' => t('There was an error during export. Operation not finished.'),
  );

  batch_set($batch);
}
