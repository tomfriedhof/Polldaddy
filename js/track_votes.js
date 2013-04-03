
/**
 * @file
 * Polldaddy vote tracking.
 */

(function ($) {
  Drupal.behaviors.polldaddyVotes = {
    attach: function (context) {
      var cookieName = 'polldaddyVotes' + window.location.pathname.replace("/", "-");
      var self = this;
      var polldaddyVotes = $.cookie(cookieName) ? JSON.parse($.cookie(cookieName)) : {};
      
      $.each(polldaddyVotes, function(index, value) {
        callback = 'PD_vote' + value;
        if (window[callback]) {
          window[callback].call(this, 1);
        }
      });
      
      $('.PDS_Poll .pds-vote-button').click(function() {
        var buttonPrefix = "pd-vote-button";
        var pollId = this.id.substr(buttonPrefix.length);
        // Blindly adding polls to a cookie, but they should only be added only
        // if they haven't been already.
        polldaddyVotes[pollId] = pollId;
        $.cookie(cookieName, JSON.stringify(polldaddyVotes), { expires: 1, path: '/' });
      });
      
      
    }
  }
})(jQuery);
