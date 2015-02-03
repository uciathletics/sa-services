<?php
define('UCINETID_LDAP_BASE_DN', 'ou=University of California Irvine,o=University of California,c=US');
define('UCINETID_LDAP_SERVER', 'ldap.service.uci.edu');

/**
 * @file
 * The UCInetID module, which allows UCI WebAuth login
 */

/**
 * Performs an HTTP 302 redirect (for the 1.x protocol).
 */
function ucinetid_redirect_http($url, $message) {
  $query = array();
  foreach ($message as $key => $val) {
    $query[] = $key .'='. urlencode($val);
  }

  $sep = (strpos($url, '?') === FALSE) ? '?' : '&';
  header('Location: '. $url . $sep . implode('&', $query), TRUE, 302);
  exit;
}

/**
 * Convert a direct communication message
 * into an associative array.
 */
function ucinetid_parse_message($message) {
  $parsed_message = array();

  $items = explode("\n", $message);
  foreach ($items as $item) {
    $parts = explode('=', $item, 2);

    if (count($parts) == 2) {
      $parsed_message[$parts[0]] = $parts[1];
    }
  }

  return $parsed_message;
}


function _ucinetid_get_params($str) {
  $chunks = explode("&", $str);

  $data = array();
  foreach ($chunks as $chunk) {
    $parts = explode("=", $chunk, 2);

    if (count($parts) == 2) {
      list($k, $v) = $parts;
      $data[$k] = urldecode($v);
    }
  }
  return $data;
}

/**
 * define this only if ldap_connect() is available
 */


/**
 * @param $ucinetid string
 * @return array of result values
 */
function ucinetid_ldap_search($ucinetid) {
  $result = array();

  if (function_exists('ldap_connect')) {
    // basic sequence with LDAP is connect, bind, search, 
    // interpret search result, close connection

    if ($ds = ldap_connect(variable_get('ucinetid_ldap_server', UCINETID_LDAP_SERVER))) {
      ldap_set_option($ds, LDAP_OPT_PROTOCOL_VERSION, 3);
      if ($r = ldap_bind($ds)) {    // this is an "anonymous" bind, typically

  /*
  *      if (is_array($ucinetid))
  *        build search criteria like (&(k1=v1)(k2=v2))
  *      else
  */

	$s = variable_get('ucinetid_ldap_uid', 'uid') .'='. ldap_quote($ucinetid);

	if ($sr = ldap_search($ds, variable_get('ucinetid_ldap_base_dn', UCINETID_LDAP_BASE_DN), "($s)")) {
	  if (ldap_count_entries($ds, $sr) == 1) {

	    $info = ldap_get_entries($ds, $sr);
	    for ($i=0; $i<$info["count"]; $i++) {       // count better be 1
	      for ($att = 0; $att < $info[$i]['count']; $att++) {
		$data = $info[$i][$att];
		if ($info[$i][$data]['count'] <= 1)
		  $result[$data] = $info[$i][$data][0];
		else
		  $result[$data] = $info[$i][$data];
	      }
	    }
	  } // ldap_count_entries
	} // ldap_search
      } // ldap_bind
      ldap_close($ds);
    } // ldap_connect
    if (isset($result['ucinetid']) && is_string($result['ucinetid']))
      $result['ucinetid'] = strtolower($result['ucinetid']);
  }
  return $result;
}

/**
 * UCI LDAP name/ucinetid keyword search
 *
 * @return array of results
 */
function ucinetid_ldap_search_all($query) { // $query: keyword keyword ... (name or UCInetID)
  $results = array();

  if (function_exists('ldap_connect')) {
    // basic sequence with LDAP is connect, bind, search, 
    // interpret search result, close connection

    if ($ds = ldap_connect(variable_get('ucinetid_ldap_server', UCINETID_LDAP_SERVER))) {
      ldap_set_option($ds, LDAP_OPT_PROTOCOL_VERSION, 3);
      if ($r = ldap_bind($ds)) {    // this is an "anonymous" bind, typically

        $a = array(); /* build search criteria like (&(k1=v1)(k2=v2)) */
        // split the phrase by any number of commas or space characters,
        // which include " ", \r, \t, \n and \f
        $keywords = preg_split("/[\s,]+/", $query);
        foreach ($keywords as $word) {
          $word = ldap_quote(trim($word));
          $c = array();
          $c[] = "(sn=$word)";
          $c[] = "(gn=$word)";
          $c[] = "(gn=* $word)";
          $c[] = "(gn=$word *)";
          $c[] = "(". variable_get('ucinetid_ldap_uid', 'uid') ."=$word)";

          $a[] = '(|'. join('',$c) .')';
        }
        $s = join('', $a); 
        if (count($a) > 1) {
          $s = "(&$s)";
        }

        if ($sr = @ldap_search($ds, variable_get('ucinetid_ldap_base_dn', UCINETID_LDAP_BASE_DN), $s)) {
            $errno = ldap_errno($ds);
            $error = ldap_error($ds);

            ldap_sort($ds, $sr, 'lastfirstname');

            $info = ldap_get_entries($ds, $sr);
            $results = array();
            for ($i=0; $i<$info["count"]; $i++) {       // count better be 1
              $result = array();
              for ($att = 0; $att < $info[$i]['count']; $att++) {
                $data = $info[$i][$att];
                $result[$data] = $info[$i][$data][0]; // just return first result only!
              }
              $results[] = $result;
            }

            if ($errno) {
              if ($errno == 4) { // too may results
                drupal_set_message('Too many matches found, some results not shown', 'warning');
              }
              else {
                drupal_set_message($error, 'error');
              }
            }
        } // ldap_search
      } // ldap_bind
      ldap_close($ds);
    } // ldap_connect
  }
  return $results;
}

//
/**
 * quote special chars in ldap query string
 *
 * @param $s string
 * @return string
 *
 * need to quote: backslash, left-paren, right-paren, asterisk, slash, null
 * http://www.ietf.org/rfc/rfc2254.txt
 */
function ldap_quote($s) {
  static $k = array('@\x5c@', '@\x28@', '@\x29@', '@\x2a@', '@\x2f@', '@\x00@', );
  static $v = array('\\\\5c', '\\\\28', '\\\\29', '\\\\2a', '\\\\2f', '\\\\00', );

  return preg_replace($k, $v, $s);
} 

/**
 * Return an array of token to value mappings for username and email address
 *
 * @param $response webauth response array from ucinetid_get_credentials()
 * @return array of mappings from token names to values (for use with strtr()).
 */
function ucinetid_tokens($arg) {
  if (is_null($arg)) {		// return example of token names/values
    $ldap = drupal_map_assoc(array('ucinetid', 'mail', 'sn', 'givenname', 'firstlastname'));
    $arg = $ldap['ucinetid'];
  }
  elseif (is_array($arg)) {
    $ldap = $arg; 		// arg was a result array
    $arg = $ldap['ucinetid'];
  }
  else
    $ldap = ucinetid_ldap_search($arg);	// might return array() if not implemented
    
  $tokens = array(
    '!ucinetid'      => strtolower($arg),
    '!mail'          => isset($ldap['mail'])          ? $ldap['mail']          : ($arg.'@uci.edu'),
    '!sn'            => isset($ldap['sn'])            ? $ldap['sn']            : '',
    '!givenname'     => isset($ldap['givenname'])     ? $ldap['givenname']     : '',
//    '!lastfirstname' => isset($ldap['lastfirstname']) ? $ldap['lastfirstname'] : '',
    '!firstlastname' => isset($ldap['lastfirstname']) ? ucinetid_pretty_name($ldap['lastfirstname']) : $arg,
  );
  return $tokens;
}

function ucinetid_pretty_name($s) {
    if (sizeof($a = explode(',', $s)) == 2)
        return trim("$a[1] $a[0]");
    return $s;
}

