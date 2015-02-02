The ucinetid module provides an external authentication capability
for Drupal.  It relies on the 'authmap' distributed authentication
facility provided by Drupal core, and was based in part on the Drupal
OpenID module.

This allows login using a cookie-based authentication system called UCI
WebAuth, developed at the University of California, Irvine (uci.edu).
For more information on UCI WebAuth, see http://oit.uci.edu/help/webauth/


General features:

A UCI WebAuth login page and login block are provided.  Users with
UCInetIDs associated with their accounts can login securely with their
UCInetID and password using UCI WebAuth.

Administrators can add UCInetID credentials to new accounts on account
creation (user_register_form), and can edit the UCInetID credentials
attached to any user account (user_profile_form).

Users can add or remove their UCInetID credentials from their own
Drupal account.

If LDAP is available:

To add new UCI users, administrators can search the UCI directory by
name or UCInetID and select a user from the search results.  The new
account creation form is pre-filled with name, email address, and
UCInetID information.

Administrators can create new accounts in a batch by entering a list of
UCInetIDs (validated with LDAP lookup).

Optional features (based on configuration settings):

Visitor account registration policy, and email validation policy can
be configured separately for UCInetID users.  Authenticated UCInetID
users can have accounts created automatically.

The Drupal login page and login block can be altered to provide a
UCI WebAuth login form alternative (shown/hidden using Javascript).
UCI WebAuth login can set as the default login method, with standard
Drupal login as an alternative.

Note: it is strongly recommended to bypass email validation and account
notification for new UCInetID accounts.  The one-time link sent by Drupal
prompts users to update their Drupal account password, and this often
confuses UCInetID users.


Contact:
John Romine
jromine@uci.edu
