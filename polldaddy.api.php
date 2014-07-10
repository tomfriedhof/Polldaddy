<?php
/**
 * @file
 * Provides functions for interacting directly with polldaddy API.
 */

function polldaddy_send_request($xml){
  try {
    $fp = fsockopen('api.polldaddy.com', 80, $err_num, $err_str, 3);
    if(!$fp) {
      throw new Exception($err_num . ': ' . $err_str);
    }
  }
  catch(Exception $e) {
    watchdog('polldaddy', "Failed to connect to api.polldaddy.com with message: '{$e->getMessage()}'", NULL, WATCHDOG_ERROR);
    return NULL;
  }

  if(function_exists('stream_set_timeout')){
    stream_set_timeout($fp, 3);
  }

  $request  = "POST / HTTP/1.0\r\n";
  $request .= "Host: api.polldaddy.com\r\n";
  $request .= "User-agent: PollDaddy PHP Client/0.1\r\n";
  $request .= "Content-Type: text/xml; charset=utf-8\r\n";
  $request .= 'Content-Length: ' . strlen($xml) . "\r\n";

  fwrite($fp, "$request\r\n$xml");

  $response = '';
  while (!feof($fp)){
    $response .= fread($fp, 4096);
  }
  fclose($fp);

  if(!$response){
    $errors[-2] = 'No Data';
  }

  return $response;
}

function polldaddy_get_usercode($APIKey){
  $xml = '<?xml version="1.0" encoding="utf-8" ?>
  <pd:pdAccess partnerGUID="'.$APIKey.'" partnerUserID="0" xmlns:pd="http://api.polldaddy.com/pdapi.xsd">
      <pd:demands>
          <pd:demand id="GetUserCode"/>
      </pd:demands>
  </pd:pdAccess>';
  $response = polldaddy_send_request($xml);
  $response = polldaddy_clear_request($response);
  $parsed = polldaddy_parse_response($response);
  
  $usercode = '';
  for($i = 0; $i < 6; $i++){
    if($parsed[$i]['tag'] == 'PD:USERCODE'){
      $usercode = $parsed[$i]['value'];
    }
  }
  return $usercode;
}

/**
 * Creates or updates a poll on polldaddy.
 *
 * @param $op
 *   Create of Update.
 * @param $settings
 *   Polldaddy settings for connecting with API. Keyed array with at least a
 *   `polldaddy_partner_guid` key containing API key.
 * @param $usercode
 *   The usercode returned from polldaddy to send with request.
 *
 * @todo: The xml request in this needs to be abstracted, so that polls can be
 *   customized to a users content.
 */
function polldaddy_save_poll($op, $settings, $usercode) {
  $pollid = $settings['polldaddy_pollid'] ? ' id="' . $settings['polldaddy_pollid'] .'"' : '';
  
  // Generate the answers section xml
  $answer_xml = '';
  foreach ($settings['polldaddy_answers'] as $answer_id => $answer) {
    // If the answer is above 1000, it's safe to consider it being an id.
    $answer_attribute = $answer_id > 1000 ? ' id="' . $answer_id . '"' : '';
    $answer_xml .= "<pd:answer$answer_attribute>
      <pd:text>$answer</pd:text>
    </pd:answer>";
  }

  $xml = <<<XMLREQ
<?xml version="1.0" encoding="utf-8" ?>
<pd:pdRequest xmlns:pd="http://api.polldaddy.com/pdapi.xsd" partnerGUID="{$settings['polldaddy_partner_guid']}">
  <pd:userCode>$usercode</pd:userCode>
  <pd:demands>
    <pd:demand id="{$op}Poll">
      <pd:poll$pollid>
        <pd:question>{$settings['polldaddy_name']}</pd:question>
        <pd:multipleChoice>no</pd:multipleChoice>
        <pd:randomiseAnswers>no</pd:randomiseAnswers>
        <pd:otherAnswer>no</pd:otherAnswer>
        <pd:resultsType>percent</pd:resultsType>
        <pd:blockRepeatVotersType>cookie</pd:blockRepeatVotersType>
        <pd:blockExpiration>7257600</pd:blockExpiration>
        <pd:comments>off</pd:comments>
        <pd:makePublic>no</pd:makePublic>
        <pd:closePoll>yes</pd:closePoll>
        <pd:closeDate>{$settings['polldaddy_closedate']}</pd:closeDate>
        <pd:styleID>{$settings['polldaddy_style_id']}</pd:styleID>
        <pd:packID>11577</pd:packID>
        <pd:folderID>140644</pd:folderID>
        <pd:languageID>30</pd:languageID>
        <pd:sharing>no</pd:sharing>
        <pd:results_order_by>{$settings['polldaddy_results_order_by']}</pd:results_order_by>
        <pd:answers>
          $answer_xml
        </pd:answers>
      </pd:poll>
    </pd:demand>
  </pd:demands>
</pd:pdRequest>
XMLREQ;

  $response = polldaddy_send_request($xml);
  $response = polldaddy_clear_request($response);
  return polldaddy_parse_response($response);
}

/**
 * Creates or updates a rating on polldaddy.
 *
 * @param $op
 *   Create of Update.
 * @param $settings
 *   Polldaddy settings for connecting with API. Keyed array with at least a
 *   `polldaddy_partner_guid` key containing API key.
 * @param $usercode
 *   The usercode returned from polldaddy to send with request.
 *
 * @todo: The xml request in this needs to be abstracted, so that polls can be
 *   customized to a users content.
 *
 * @return function see polldaddy_parse_response().
 */
function polldaddy_save_rating($op, $settings, $usercode) {
  $pollid = $settings['polldaddy_pollid'] ? ' id="' . $settings['polldaddy_pollid'] .'"' : '';

  $xml = <<<XMLREQ
<?xml version="1.0" encoding="utf-8" ?>
<pd:pdRequest xmlns:pd="http://api.polldaddy.com/pdapi.xsd" partnerGUID="{$settings['polldaddy_partner_guid']}">
  <pd:userCode>$usercode</pd:userCode>
  <pd:demands>
    <pd:demand id="{$op}Rating">
      <pd:rating id="{$settings['polldaddy_pollid']}" type='0'>
        <pd:date>{$settings['date']}</pd:date>
        <pd:name><![CDATA[{$settings['polldaddy_name']}]]></pd:name>
        <pd:folder_id>{$settings['folder_id']}</pd:folder_id>
        <pd:settings><![CDATA[{
        "type" : "{$settings['type']}",
        "size" : "{$settings['size']}",
        "star_color" : "{$settings['star_color']}",
        "custom_star" : "{$settings['custom_star']}",
        "font_size" : "12px",
        "font_line_height" : "{$settings['font_line_height']}",
        "font_color" : "#333333",
        "font_align" : "left",
        "font_position" : "right",
        "font_family" : "verdana",
        "font_bold" : "normal",
        "font_italic" : "normal",
        "text_vote" : "Vote",
        "text_votes" : "Votes",
        "text_rate_this" : "{$settings['text_rate_this']}",
        "text_1_star" : "Very Poor",
        "text_2_star" : "Poor",
        "text_3_star" : "Average",
        "text_4_star" : "Good",
        "text_5_star" : "Excellent",
        "text_thank_you" : "{$settings['text_thank_you']}",
        "text_rate_up" : "{$settings['text_rate_up']}",
        "text_rate_down" : "{$settings['text_rate_down']}"
        }]]></pd:settings>
      </pd:rating>
    </pd:demand>
  </pd:demands>
</pd:pdRequest>
XMLREQ;

  $response = polldaddy_send_request($xml);
  $response = polldaddy_clear_request($response);
  return polldaddy_parse_response($response);
}

function polldaddy_parse_response($response){
  $xml_parser = xml_parser_create();
  $data = array();
  xml_parse_into_struct($xml_parser, $response, $data);
  return $data;
}

function polldaddy_clear_request($response){
  return preg_replace("/[ a-zA-Z0-9:;\n\r\-=\/,\.]*</",'<',$response,1);
}

function polldaddy_get_polls(){
  $settings = variable_get('polldaddy_settings', array());
  $xml = '<?xml version="1.0" encoding="utf-8" ?>
  <pd:pdRequest xmlns:pd="http://api.polldaddy.com/pdapi.xsd" partnerGUID="'.$settings['polldaddy_partner_guid'].'">
      <pd:userCode>'.$settings['polldaddy_usercode'].'</pd:userCode>
      <pd:demands>
          <pd:demand id="GetPolls">
            <pd:list end="0" start="0"/>
          </pd:demand>
      </pd:demands>
  </pd:pdRequest>';
  $response = polldaddy_send_request($xml);
  $response = polldaddy_clear_request($response);
  $parsed = polldaddy_parse_response($response);
  $polls = array('None' => '- None selected -');
  foreach($parsed as $poll){
    if($poll['tag'] == 'PD:POLL'){
      $polls[$poll['attributes']['ID']] = $poll['value'];
    }
  }

  return $polls;
}
