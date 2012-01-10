$(function() {
    var sock = new io.connect('http://' + window.location.host),
    peaks = new io.connect('http://' + window.location.host + '/peaks'),
    ping = new io.connect('http://' + window.location.host + '/ping');

    peaks.on('message', function(data) {
               var d = $.parseJSON(data[0]);
               lightVU(d);
               $('#maxL').html(maxL);
               $('#maxR').html(maxR);
             });


    function getPrintableDate(date) {
      return date.getFullYear().toString() + '/' +
        (date.getMonth()+1).toString() + '/' +
        date.getDate().toString() + ' ' +
        date.getHours().toString() + ':' +
        date.getMinutes().toString() + ':' +
        date.getSeconds().toString() + '.' +
        date.getMilliseconds().toString();
    }

    function encodeDate(date)
    {
      return [date.getHours(), date.getMinutes(), date.getSeconds(), date.getMilliseconds()];
    }

    function decodeDate(data)
    {
      var date = new Date();
      return new Date(date.getFullYear(), date.getMonth(), date.getDate(),
                      data[0], data[1], data[2], data[3]);
    }

    // Ping
    ping.on('message', function(data) {
              var client = decodeDate(data.client);
              var server = decodeDate(data.server);
              var now = new Date();

              $('#ping').html('Ping: ' + (now.getTime() - client.getTime()).toString() + ' ms.<br/>');
            });

    function sendPing()
    {
      ping.json.send({client: encodeDate(new Date()) });
      setTimeout(sendPing, 5000);
    }
    sendPing();

    //send the message when submit is clicked
    $('#chatform').submit(function (evt) {
                            var line = $('#chatform [type=text]').val()
                            $('#chatform [type=text]').val('')
                            peaks.send(line);
                            return false;
                          });
    $('#reset').click(function(evt){resetVUMax();});
  });
