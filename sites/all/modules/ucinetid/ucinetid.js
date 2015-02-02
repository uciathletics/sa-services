/**
 * ucinetid.js
 */
(function ($) {

Drupal.behaviors.ucinetid = {
 attach: function(context, settings)  {
  var fuser = $('#user-login, #user-login-form');
  var fuci = $('#ucinetid-login-wrapper');

  if (fuser.length && fuci.length) {
    var tabs = $('.tabs:first'); // primary tabs

    var l = $('#ucinetid-links a', context).click(function() { // login using ucinetid
      fuser.hide();
      tabs.hide();
      /* $("div.messages.error").hide(); */
      fuci.show();
      $("#edit-ucinetid").focus();
      return false;
    });

    $('#ucinetid-user-login-cancel a', context).click(function() {  // cancel ucinetid login
      tabs.show();
      fuser.show();
      /* $("div.messages.error").show(); */
      fuci.hide();
      $("#edit-name").focus();
      return false;
    });

    if (l && settings.ucinetid.login) {
      if (!$("#edit-name").hasClass('error')) {
        l.click();
      }
    }

  }
 }
};

})(jQuery);
