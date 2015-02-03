<?php

/**
 * @file
 * The UCInetID module, which allows UCI WebAuth login
 */

/**
 * Menu callback.
 *
 * callback on /user/%user/ucinetid/{add,delete,return}
 * handles click on 'add ucinetid' or 'delete ucinetid' buttons
 */
function ucinetid_user_edit_callback($account, $op = '') {
  module_load_include('inc', 'ucinetid');

  $authmap = ucinetid_get_authmap($account);    // see if ucinetid authname present

  if ($op == 'add' || $op == 'return') {

    if (isset($authmap['aid'])) {               // authmap already present
      drupal_set_message(t('UCInetID already present'), 'error');
    }
    else {
      $response = ucinetid_get_credentials();   // lookup/validate ucinetid credentials

      if (!isset($response['error']) && isset($response['ucinetid']) && $response['ucinetid'] != '') {

        $a2 = ucinetid_get_authmap($response);  // check if ucinetid in use
        if (isset($a2['aid']))
          drupal_set_message(t('UCInetID %ucinetid already assigned to ',
            array('%ucinetid' => $response['ucinetid'])) .  l(t('another account'), 'user/'. $a2['uid']) .'.' , 'error');
        else {
          // valid credentials, add 'em
          user_set_authmaps($account, array('authname_ucinetid' => ucinetid_authname($response)));
          drupal_set_message(t('UCInetID %ucinetid added', array('%ucinetid' => $response['ucinetid'])));
        }
      }
      elseif ($op == 'add') {                   // no credentials, go try webauth login and return
        $return_url = url('user/'. $account->uid .'/ucinetid/return', array('absolute' => TRUE ));
        ucinetid_redirect_http(variable_get('ucinetid_login_url', UCINETID_WEBAUTH_LOGIN), array('return_url' => $return_url));
        /* NOTREACHED */
      }

      // no credentials, sorry
      else
        drupal_set_message(t('UCInetID authentication failed: @error', array('@error' => $response['error'])), 'error');
    }
  }
  elseif ($op == 'delete') {
    if (isset($authmap['aid'])) {
        user_set_authmaps($account, array('authname_ucinetid' => NULL));
        drupal_set_message(t('UCInetID removed'));
    }
    else
      drupal_set_message(t('No UCInetID to remove'), 'error');
  }
  drupal_goto('user/'. $account->uid .'/edit');
}


/**
 * callback on user/login/ucinetid (login using ucinetid)
 *
 * This gets called prior to ($op=login) and after ($op=return) 
 * the user has authenticates with WebAuth.
 *
 * @param $op 3rd argument ('return' on callback from webauth login)
 */
function ucinetid_authenticate($op = 'login') {        // 'login' or 'return'
  /*
   * ucinetid_login_page (function of /user/login/ucinetid page):
   *  0: Check for WebAuth credentials, and redirect to OIT WebAuth page if not authenticated
   *  1: Display an embedded WebAuth login form directly
   *
   * ucinetid_login (what to show on /user/login page & block):
   *  0: Show the regular Drupal login form by default
   *  1: Show the UCInetID login form by default
   */

  if (empty($op) && variable_get('ucinetid_login_page', 0) == 1) { // show login page
    return drupal_get_form('ucinetid_login'); // login form
  }

  $response = ucinetid_get_credentials();       // read COOKIE and get webauth_check() values

  /* if error, then not logged in with webauth */
  if (isset($response['error'])) {
    if ($op == 'return') {              // 2nd try: tried webauth login and still no good, so punt
      drupal_set_message(t('UCInetID login failed: @error', array('@error' => $response['error'])), 'error');
      drupal_goto('user/login');
    }

    // not already logged in; go try webauth login and return here (2nd try)
    $return_url = url('user/login/ucinetid/return', array('absolute' => TRUE,
      'query' => (isset($_GET['destination']) ? drupal_get_destination() : NULL),
    ));

    // go try webauth login, and come back when done
    ucinetid_redirect_http(variable_get('ucinetid_login_url', UCINETID_WEBAUTH_LOGIN), array('return_url' => $return_url));
  }

  // at this point, we have webauth credentials

  $authname = $response['authname'];
  $account = user_external_load($authname);     // lookup authname, do a user_load()

  /* user_register:
   *  0: only admin can create accounts
   *  1: visitors can create accounts w/o admin approval
   *  2: visitors can create accounts but admin approval is required
   * user_email_verification:
   *  0: not required, show 'password' input boxes on user_register form
   *  1: required, generate random password, and mail it to new user
   */

  if (isset($account->uid)) {                   // found an account matching authname
    if ($account->status == 0 && $account->login == 0)  // blocked, not logged in
      drupal_set_message(t('Your account is currently pending approval by the site administrator.'));
    elseif (!variable_get('user_email_verification', TRUE) || $account->login || !variable_get('ucinetid_user_email_verification', 1)) { // no email verification, or already logged in
      // Check if user is blocked.
      $state['values']['name'] = $account->name;
      user_login_name_validate(array(), $state);
      if (!form_get_errors()) {
        $form_state['uid'] = $account->uid;
        user_login_submit(array(), $form_state);  // and just like that, we're logged in
      }
    }
    else {
      drupal_set_message(t('You must validate your email address for this account before logging in via ucinetid'));
    }
  }
  // check if these variables are non-zero
  elseif (variable_get('ucinetid_user_register', 0) || variable_get('user_register', USER_REGISTER_VISITORS_ADMINISTRATIVE_APPROVAL)) {

    // we have an authname (not yet saved), but no account matched it
    // visitors (or authenticated ucinetid visitors) can register accounts

    /*
     * ucinetid_user_register:
     * 0: follow standard setting
     * 1: yes
     * 2: yes, with admin approval
     */

    // check if proposed account is available
    $tokens = ucinetid_tokens($response['ucinetid']);
    $name = format_string(variable_get('ucinetid_name', '!ucinetid'), $tokens);
    $mail = format_string(variable_get('ucinetid_mail', '!mail'), $tokens);

    if ($u = user_load_by_name($name)) {
      drupal_set_message(t('An account with the username %username is already registered on this site.  If this is your account, !login with your username and password and add your UCInetID under "My Account".', array('%username' => theme('username', array('account' => $u)), '!login' => l(t('login'), 'user/login'))), 'error');
    }
    elseif ($m = user_load_by_mail($mail)) {
      drupal_set_message(t('An account with the email address %mail is already registered on this site.  If this is your account, !login with your username and password and add your UCInetID under "My Account".', array('%mail' => $mail, '!login' => l(t('login'), 'user/login'))), 'error');
    }
    else {
      // looks like the account is probably available to register

      // this code duplicates user_external_login_register(), with a few
      // changes.  Unfortunately, that function doesn't give us enough
      // flexibility to use it directly.

      $userinfo = array(
        'name' => $name,
        'pass' => user_password(),
        'init' => $name,
        'mail' => $mail,
        'status' => variable_get('ucinetid_user_register', 0) == USER_REGISTER_VISITORS || variable_get('user_register', USER_REGISTER_VISITORS_ADMINISTRATIVE_APPROVAL) == USER_REGISTER_VISITORS, // check if these == 1
        'access' => REQUEST_TIME,
      );

      $account = user_save(drupal_anonymous_user(), $userinfo);
      if ($account) {
        user_set_authmaps($account, array("authname_ucinetid" => $authname));

        if ($account->status) { // account in active
          // Log user in.
          $form_state['uid'] = $account->uid;
          user_login_submit(array(), $form_state);  // and now we're registered and logged in

          drupal_set_message(t('An account was created for you.  Click on !profile to view or edit your account profile.', array('!profile' => l(t('My account'), 'user'))));
        }
        else {
          drupal_set_message(t('An account was created for you.  Your account is currently pending approval by the site administrator.'));
        }
      }
      else {
        drupal_set_message(t('Creating a account for you failed for the reasons listed above. If you already have an account, !login now and add your UCInetID under "My Account"', array(l(t('login'), 'user/login'))), 'error');
      }
    }
  }
  else {
    if (!empty($response['ucinetid'])) {
      drupal_set_message(t('Greetings %ucinetid.', array('%ucinetid' => $response['ucinetid'])), 'error');
    }
    drupal_set_message(t('You do not appear to have an account on this site, and only administrators may register new accounts.'), 'error');
  }
  drupal_goto();
}

/**
 * Lookup/validate current UCInetID identity (cookie)
 *
 * @return array(
 *  'ucinetid' => jromine,
 *  'campusid' => 000000060962,
 *  'authname' => jromine@login.uci.edu/000000060962,
 *  'auth_host' => 128.195.173.37,
 *  'time_created' => 1235693387,
 *  'last_checked' => 1235693387,
 *  'max_idle_time' => 7,
 *  'login_timeout' => n,
 *  'age_in_seconds' => 7,
 *  'uci_affiliations' => staff,employee,
 *  'seconds_since_checked' => 7,
 *  'error' => string - if errors )
 */
function ucinetid_get_credentials() {
  module_load_include('inc', 'ucinetid');

  if (!isset($_COOKIE) || !is_array($_COOKIE))
    $err = 'no cookie found';
  elseif (!isset($_COOKIE['ucinetid_auth']) || $_COOKIE['ucinetid_auth'] == '')
    $err = 'no ucinetid_auth cookie found';
  elseif (!($result = drupal_http_request(url(variable_get('ucinetid_validate_url', UCINETID_WEBAUTH_CHECK), array('query' => array("ucinetid_auth" => $_COOKIE['ucinetid_auth']))))))
    $err = 'http request failed';
  elseif (isset($result->error))
    $err = $result->error;
  elseif (!($response = ucinetid_parse_message($result->data)))
    $err = "can't parse http response";
  elseif (isset($response['auth_fail']))
    $err  = $response['auth_fail'];
  elseif (!isset($response['ucinetid']) || $response['ucinetid'] == '')
    $err = 'no ucinetid in response';
  elseif (!isset($response['auth_host']) || $response['auth_host'] == '')
    $err = 'no auth_host in response';
  elseif (!isset($_SERVER['REMOTE_ADDR']) || $_SERVER['REMOTE_ADDR'] == '')
    $err = 'no remote_addr found';
  elseif ($response['auth_host'] != $_SERVER['REMOTE_ADDR'])
    $err = "auth_host doesn't match remote_addr";
  elseif (!isset($response['campus_id']) || ($response['campusid'] = $response['campus_id']) == '')
    $err = 'no campusid found in response';

  /* elseif (an unreasonable amount of time has passed since login) $err = 'sessions timeout' */
  else
    $response['authname'] = ucinetid_authname($response);

  if (!isset($response) || !is_array($response))
    $response = array('error' => 'invalid response');

  if (isset($err))
    $response['error'] = $err;

  return $response;
}

/**
 * UCInetID Batch Add form.
 *
 * Enter a batch of UCInetIDs and register accounts for all of them at once.
 */
function ucinetid_batch_register($form, &$form_state, $arg = NULL) {
  $form['ucinetid'] = array(
    '#type' => 'textarea',
    '#default_value' => '',
    '#title' => t('UCInetIDs'),
    '#required' => TRUE,
    '#description' => t('Enter one or more UCInetIDs to create new user accounts. Multiple UCInetIDs should be separated by a newline or comma.'),
  );
  $form['account'] = array(
    '#type' => 'value',
    '#value' => NULL,                // a place to stuff looked-up accounts from validate to submit
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit', 
    '#value' => t('Create new account'), 
    '#weight' => 30
  );

  return $form;
}

/*
 * Implements form validate for ucinetid_batch_register().
 *
 * Verify lookup of UCInetIDs, and stuff results into $form['account'][]
 */
function ucinetid_batch_register_validate($form, &$form_state) {
  module_load_include('inc', 'ucinetid');
  $err = 0;

  $names = preg_split("/[\s,]+/", $form_state['values']['ucinetid']);
  $account = array();
  foreach ($names as $ucinetid) {
    $ucinetid = trim($ucinetid);
    if (strlen($ucinetid) == 0)
      continue;

    $response = ucinetid_ldap_search($ucinetid);
    if (empty($response) || empty($response['ucinetid']) || empty($response['campusid'])) {
      $err++;
      drupal_set_message(t('The UCInetID %ucinetid was not found in the directory', array('%ucinetid' => $ucinetid)), 'error');
      continue;
    }

    $a2 = ucinetid_get_authmap($response);
    if (isset($a2['uid'])) {
      $err++;
      drupal_set_message(t('UCInetID %ucinetid is assigned to ', 
        array('%ucinetid' => $response['ucinetid'])) .  l(t('another account'), 'user/'. $a2['uid']) .'.', 'error');
      continue;
    }

    $tokens = ucinetid_tokens($response);	// response is LDAP result
    $name = format_string(variable_get('ucinetid_name', '!ucinetid'), $tokens);
    if (strlen($name) == 0) {
      $err++;
      drupal_set_message(t('The username for UCInetID %ucinetid is blank; check the UCInetID Login module settings',
        array('%ucinetid' => $ucinetid)), 'error');
      continue;
    }
    $mail = format_string(variable_get('ucinetid_mail', '!mail'), $tokens);
    if (strlen($mail) == 0) {
      $err++;
      drupal_set_message(t('The e-mail address for UCInetID %ucinetid is blank; check the UCInetID Login module settings',
        array('%ucinetid' => $ucinetid)), 'error');
      continue;
    }

    $u = user_load_by_name($name);
    if (isset($u->uid)) {
      $err++;
      drupal_set_message(t('The username !username for UCInetID %ucinetid is already taken',
        array('!username' => theme('username', array('account' => $u)), '%ucinetid' => $ucinetid)), 'error');
      continue;
    }
    $response['name'] = $name;

    $u = user_load_by_mail($mail);
    if (isset($u->uid)) {
      $err++;
      drupal_set_message(t('The e-mail address for UCInetID %ucinetid (%mail) is already taken.',
		array('%ucinetid' => $ucinetid, '%mail' => $mail)) , 'error');
      continue;
    }
    $response['mail'] = $mail;

    $account[$ucinetid] = $response;
  }
  form_set_value(array('#parents' => array('account')), $account, $form_state); // stuff this

  if ($err) {
    form_set_error('ucinetid', format_plural($err, '1 error found', '@count errors found'));
  }
}

/*
 * Form submit handler for ucinetid_batch_register().
 *
 * Create each account with random password and set authmap.
 */
function ucinetid_batch_register_submit($form, &$form_state) {
  foreach ($form_state['values']['account'] as $response) {
    $userinfo = array(
      'name' => $response['name'],
      'pass' => user_password(),
      'init' => $response['name'],
      'mail' => $response['mail'],
      'status' => 1,
      'access' => REQUEST_TIME,
    );

    $account = user_save(drupal_anonymous_user(), $userinfo);
    if ($account->uid) {
      user_set_authmaps($account, array("authname_ucinetid" => ucinetid_authname($response)));
      $uri = entity_uri('user', $account);
      drupal_set_message(t('Created a new user account for !username. No e-mail has been sent.', array('!username' => theme('username', array('account' => $account)))));
    }
  }
}

/**
 * Form builder; Output a search form and show the results.
 *
 * Keyword search UCI LDAP directory.  Return list of accounts with quick
 * link to user_register_form().
 */
function ucinetid_ldap_search_form($form, &$form_state, $keywords, $base = 'admin/user/user', $view = 'user') {
  module_load_include('inc', 'ucinetid');

  $form['#search'] = $base .'/ucinetid/search/';
  $form['search'] = array(
    '#type' => 'fieldset',
    '#title' => t('Search'),
  );
  $form['search']['query'] = array(
    '#title' => t('Search UCI Directory'),
    '#type' => 'textfield',
    '#size' => 40,
    '#default_value' => isset($keywords) ? $keywords : '',
    '#attributes' => array('title' => t('Enter name or UCInetID keywords to search for.')),
    '#description' => t('Enter name keywords, or UCInetID to search UCI directory.'),
  );
  $form['search']['actions'] = array('#type' => 'actions');
  $form['search']['actions']['submit'] = array(
    '#type' => 'submit', 
    '#value' => t('Search')
  );

  if (!empty($keywords)) {
    $results = ucinetid_ldap_search_all($keywords);
    $form['search']['response'] = array(
      '#tree' => TRUE,
      '#theme' => 'ucinetid_user_search'
    );
    foreach ($results as $response) {
      if (is_array($response)) {
        $dept = '';
        if (isset($response['department'])) {
          $dept = $response['department'];
          if (isset($response['title']))
            $dept = $response['title'] .', '. $dept;
        }
        elseif (isset($response['major'])) {
          $dept = $response['major'];
          if (isset($response['studentlevel']))
            $dept = $response['studentlevel'] .' '. $dept;
        }
        else
          $dept = $response['type'];


        $auth = ucinetid_get_authmap($response);
        $user = !empty($auth['uid']) ? user_load($auth['uid']) : NULL;

        $f = array();
        $f['#response'] = $response;

        $tokens = ucinetid_tokens($response);
        // q = [name, mail, ucinetid, campusid]
        $q = array(
          'name' => format_string(variable_get('ucinetid_name', '!ucinetid'), $tokens),
          'ucinetid' => strtolower($response['ucinetid']),
          'campusid' => isset($response['campusid']) ? $response['campusid'] : '',
          'mail' => format_string(variable_get('ucinetid_mail', '!mail'), $tokens),
        );
        $f['add'] = array( // Add or Edit
          '#type' => 'item',
          '#title' => '',
          '#markup' => empty($user) ? l('Add', $base .'/create', array('query' => $q)) : l('Edit', $view .'/'. $user->uid),
        );
        $f['name'] = array(
          '#type' => 'item',
          '#title' => t('Name'),
          '#markup' => ucwords(strtolower($response['lastfirstname'])),
        );
        $f['user'] = array(
          '#type' => 'item',
          '#title' => t('Account'),
          '#markup' => !empty($user) ? theme('username', array('account' => $user)) : '',
        );
        $f['department'] = array(
          '#type' => 'item',
          '#title' => t('Dept'),
          '#markup' => check_plain($dept),
        );
        $f['mail'] = array(
          '#type' => 'item',
          '#title' => t('E-mail'),
          '#markup' => isset($response['mail']) ? l($response['mail'], "mailto:$response[mail]") : '',
        );
        $f['phone'] = array(
          '#type' => 'item',
          '#title' => t('Phone'),
          '#markup' => isset($response['telephonenumber']) ? check_plain($response['telephonenumber']) : '',
        );
      }
      else {
        $f = array(
          '#response' => $response,
          'name' => array(
            '#type' => 'item',
            '#markup' => check_plain($response),
            '#title' => t('Message'),
          ),
        );
      }
      $form['search']['response'][] = $f;
    }
  }

  return $form;
}

function ucinetid_ldap_search_form_submit($form, &$form_state) {
  // The search form relies on control of the redirect destination for its
  // functionality, so we override any static destination set in the request,
  // for example by drupal_access_denied() or drupal_not_found()
  // (see http://drupal.org/node/292565).
  if (isset($_GET['destination'])) {
    unset($_GET['destination']);
  }
  if (isset($_GET['edit']['destination'])) {
    unset($_GET['edit']['destination']);
  }

  // TODO: should this be urlencoded?
  if (!empty($form['#search'])) {
    $form_state['redirect'] = $form['#search'] . trim($form_state['values']['query']);
  }
}
