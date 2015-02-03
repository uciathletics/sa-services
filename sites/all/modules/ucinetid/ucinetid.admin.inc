<?php

/**
 * @file
 * Module admin page callbacks.
 */

/**
 * Implements the settings page.
 *
 * @return
 *   The form structure.
 */
function ucinetid_admin_settings($form, &$form_state) {            // admin/user/ucinetid
  $form['register'] = array(
    '#type' => 'fieldset',
    '#title' => t('New account registration'),
    '#description' => t('For new users who authenticate with their UCInetID, Drupal accounts can be automatically created.'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $a = array('no','yes','yes, with administrator approval');
  $options_user_register = array(
    0 => 'Follow standard site policy (currently: '. $a[variable_get('user_register', USER_REGISTER_VISITORS_ADMINISTRATIVE_APPROVAL)] .')',
    1 => 'Yes, with no administrator approval needed',
    2 => 'Yes, with administrator approval',
  );
  $form['register']['ucinetid_user_register'] = array(
    '#type' => 'radios',
    '#title' => t('Can new UCInetID users register accounts?'),
    '#description' => t('This allows you to set a different policy for registering new accounts for users authenticated with a UCInetID.'),
    '#default_value' => variable_get('ucinetid_user_register', 0),
    '#options' => $options_user_register,
    '#required' => TRUE,
  );

  $a = array('no','yes');
  $options_user_email_verification = array(
    1 => 'Follow standard site policy (currently: '. $a[variable_get('user_email_verification', 1)] .')',
    0 => 'No, always bypass this requirement for UCInetID users (recommended setting)',
  );
  $form['register']['ucinetid_user_email_verification'] = array(
    '#type' => 'radios',
    '#title' => t('Require email verification for new UCInetID users?'),
    '#description' => t('This allows you to set a different policy for validating new accounts for users authenticated with a UCInetID.'),
    '#default_value' => variable_get('ucinetid_user_email_verification', 1),
    '#options' => $options_user_email_verification,
    '#required' => TRUE,
  );

  $form['register']['advanced'] = array(
    '#type' => 'fieldset',
    '#collapsed' => TRUE,
    '#collapsible' => TRUE,
    '#title' => t('Automated registration'),
    '#description' => t('Configuration for automatically registering new accounts for
    UCInetID users.  You should probably accept the default values unless you know what
    you are doing.'),
  );

  $form['register']['advanced']['ucinetid_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Username'),
    '#default_value' => variable_get('ucinetid_name', '!ucinetid'),
    '#size' => 20,
    '#maxlength' => 255,
    '#description' => t('A variable to use for new account username. Choose from: !list.', array('!list' => '!ucinetid, !sn (SURNAME), !givenname, !firstlastname (First M. Last)')),
    '#required' => TRUE,
  );
  $form['register']['advanced']['ucinetid_mail'] = array(
    '#type' => 'textfield',
    '#title' => t('E-mail address'),
    '#default_value' => variable_get('ucinetid_mail', '!mail'),
    '#size' => 20,
    '#maxlength' => 255,
    '#description' => t('A variable to use for new account E-mail address. Choose from: !list.', array('!list' => '!ucinetid, !mail')),
    '#required' => TRUE,
  );


  $form['login'] = array(
    '#type' => 'fieldset',
    '#title' => t('Login settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#description' => t('The module provides a !ucilogin link on the Drupal login page and login block.', array('!ucilogin' => l('Log in with UCInetID', 'user/login/ucinetid'))),
  );
  $options = array(
    0 => 'Check for WebAuth credentials, and redirect to the '. l('OIT WebAuth login', UCINETID_WEBAUTH_LOGIN) . ' page if not authenticated.',
    1 => 'Display an embedded WebAuth login form directly.',
  );
  $form['login']['ucinetid_login_page'] = array(
    '#type' => 'radios',
    '#title' => t('UCInetID login'),
    '#default_value' => variable_get('ucinetid_login_page', 0),
    '#options' => $options,
    '#required' => TRUE,
    '#description' => t('Select the function of the !url page.', array('!url' => l('/user/login/ucinetid', 'user/login/ucinetid'))),
  );
  $options = array(
    0 => 'Show the regular Drupal login form by default.',
    1 => 'Show the UCInetID login form by default.',
  );
  $form['login']['ucinetid_login'] = array(
    '#type' => 'radios',
    '#title' => t('Login form'),
    '#default_value' => variable_get('ucinetid_login', 0),
    '#options' => $options,
    '#required' => TRUE,
    '#description' => t('Select the default login form to show on the !login page.', array('!login' => l('/user/login', 'user/login'))),
  );

  $form['webauth'] = array(
    '#type' => 'fieldset',
    '#title' => t('WebAuth settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('These settings are required for the operation of this module.  You should probably accept the default values unless you are testing with an alternate server.'),
  );
  $form['webauth']['ucinetid_login_url'] = array(
    '#type' => 'textfield',
    '#title' => t('WebAuth login page'),
    '#default_value' => variable_get('ucinetid_login_url', UCINETID_WEBAUTH_LOGIN),
    '#size' => 80,
    '#maxlength' => 255,
    '#description' => t('The page that prompts the user for a UCInetID and password.'),
    '#required' => TRUE,
  );
  $form['webauth']['ucinetid_validate_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Validate URL'),
    '#default_value' => variable_get('ucinetid_validate_url', UCINETID_WEBAUTH_CHECK),
    '#size' => 80,
    '#maxlength' => 255,
    '#description' => t('The page that validates the WebAuth cookie and returns identity data.'),
    '#required' => TRUE,
  );

  $form['ldap'] = array(
    '#type' => 'fieldset',
    '#title' => t('LDAP settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('If an LDAP server is available, this module will use it to lookup CampusID numbers corresponding to UCInetIDs, and also to lookup full name and E-mail addresses for use in new account creation.  These settings are recommended, but optional.'),
  );
  $form['ldap']['ucinetid_ldap_server'] = array(
    '#type' => 'textfield',
    '#title' => t('Server'),
    '#default_value' => variable_get('ucinetid_ldap_server', 'ldap.service.uci.edu'),
    '#size' => 80,
    '#maxlength' => 255,
    '#description' => t('The domain name or IP address of the LDAP server.'),
  );
  $form['ldap']['ucinetid_ldap_base_dn'] = array(
    '#type' => 'textfield',
    '#title' => t('Base DN'),
    '#default_value' => variable_get('ucinetid_ldap_base_dn', 'ou=University of California Irvine,o=University of California, c=US'),
    '#size' => 80,
    '#maxlength' => 255,
    '#description' => t('The LDAP Base Distinguised Name.'),
  );
  $form['ldap']['ucinetid_ldap_uid'] = array(
    '#type' => 'textfield',
    '#title' => t('UID field'),
    '#default_value' => variable_get('ucinetid_ldap_uid', 'ucinetid'),
    '#size' => 80,
    '#maxlength' => 255,
    '#description' => t('The LDAP User ID field.'),
  );
  if (!function_exists('ldap_connect')) {
    $form['ldap']['#description'] = t('LDAP integration is not currently available, because PHP isn\'t loaded with the necessary LDAP libraries.  You may need to load the <em>php-ldap</em> package on your server.<br>  See <a href="@url">@url</a> for more information.', array('@url' => 'http://www.php.net/manual/en/book.ldap.php'));
    $form['ldap']['ucinetid_ldap_server']['#access'] = FALSE;
    $form['ldap']['ucinetid_ldap_base_dn']['#access'] = FALSE;
    $form['ldap']['ucinetid_ldap_uid'] ['#access'] = FALSE;
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type'  => 'submit',
    '#value' => t('Save configuration'),
  );
  $form['actions']['reset'] = array(
    '#type'  => 'submit',
    '#value' => t('Reset to defaults'),
  );
  return $form;
}

function ucinetid_admin_settings_validate($form, $form_state) {
  module_load_include('inc', 'ucinetid');
  $tokens = ucinetid_tokens(NULL);

  // for now, this allows only one token in each field
  foreach (array('ucinetid_name', 'ucinetid_mail') as $k) {
    if (!array_key_exists($form_state['values'][$k], $tokens))
      form_set_error($k, 'Not a valid selection. See the list of available variables.');
  }
}

/**
 * Submit hook for the settings form.
 */
function ucinetid_admin_settings_submit($form, &$form_state) {
  $op = $form_state['triggering_element']['#value'];
  $values = $form_state['values'];

  if ($op == t('Reset to defaults')) {
    variable_del('ucinetid_login');
    variable_del('ucinetid_login_url');
    variable_del('ucinetid_login_page');
    variable_del('ucinetid_validate_url');

    variable_del('ucinetid_ldap_server');
    variable_del('ucinetid_ldap_uid');
    variable_del('ucinetid_ldap_base_dn');

    variable_del('ucinetid_user_register');
    variable_del('ucinetid_user_email_verification');
    variable_del('ucinetid_name');
    variable_del('ucinetid_mail');

    drupal_set_message(t('The configuration options have been reset to their default values.'));
  }
  else {
    variable_set('ucinetid_user_register', $values['ucinetid_user_register']);
    variable_set('ucinetid_user_email_verification', $values['ucinetid_user_email_verification']);
    variable_set('ucinetid_name', $values['ucinetid_name']);
    variable_set('ucinetid_mail', $values['ucinetid_mail']);
    variable_set('ucinetid_login', $values['ucinetid_login']);
    variable_set('ucinetid_login_url', $values['ucinetid_login_url']);
    variable_set('ucinetid_login_page', $values['ucinetid_login_page']);
    variable_set('ucinetid_validate_url', $values['ucinetid_validate_url']);
    variable_set('ucinetid_ldap_server', $values['ucinetid_ldap_server']);
    variable_set('ucinetid_ldap_base_dn', $values['ucinetid_ldap_base_dn']);
    variable_set('ucinetid_ldap_uid', $values['ucinetid_ldap_uid']);

    drupal_set_message(t('The configuration options have been saved.'));
  }
}
