
/**
 * @file
 * Polldaddy vote tracking.
 */

(function ($) {
  Drupal.behaviors.polldaddyVotes = {
    attach: function (context) {
      var self = this;
      var polldaddyVotes = $.cookie('polldaddyVotes') ? JSON.parse($.cookie('polldaddyVotes')) : {};

      $.each(polldaddyVotes, function(index, value) {
        callback = 'PD_vote' + value;
        window[callback].call(this, 1);
      });
      
      $('.PDS_Poll .pds-votebutton').click(function() {
        var buttonPrefix = "pd-vote-button";
        var pollId = this.id.substr(buttonPrefix.length);
        // Blindly adding polls to a cookie, but they should only be added only
        // if they haven't been already.
        polldaddyVotes[pollId] = pollId;
        $.cookie("polldaddyVotes", JSON.stringify(polldaddyVotes), { expires: 10000, path: '/' });
      });
      
      
    }
  }
})(jQuery);
