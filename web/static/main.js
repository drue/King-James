function zeroPad(num,count)
{
  var numZeropad = num + '';
  while(numZeropad.length < count) {
    numZeropad = "0" + numZeropad;
  }
  return numZeropad;
}

$(function() {
    var sock = new io.connect('http://' + window.location.host),
    peaks = new io.connect('http://' + window.location.host + '/peaks'),
    ping = new io.connect('http://' + window.location.host + '/ping'),
    status = new io.connect('http://' + window.location.host + '/status');

    peaks.on('message', function(data) {
               var d = $.parseJSON(data[0]);
               lightVU(d);
               $('#maxL').html(maxL);
               $('#maxR').html(maxR);
             });


    status.on('message', function(data) {
                var s = $.parseJSON(data[0]);
                var allLeft = s.r / 3600;
                var hoursLeft = Math.floor(allLeft);
                var minutesLeft = Math.floor((allLeft - hoursLeft) * 60);
                $('#timeLeft').html(hoursLeft + ":" + minutesLeft);

                var elapsed = s.t / 3600;
                var hoursElapsed = Math.floor(elapsed);
                var minutes = (elapsed - hoursElapsed) * 60;
                var minutesElapsed = Math.floor(minutes);
                var secondsElapsed = Math.floor((minutes - minutesElapsed) * 60);
                $('#timeElapsed').html(zeroPad(hoursElapsed, 2) + ":" +
                                       zeroPad(minutesElapsed, 2) + ":" +
                zeroPad(secondsElapsed, 2));

                $('#buffer').html(s.b + "s");

                $('#load').html("(" + s.c[0] + ", " + s.c[1] + ", " + s.c[2] + ")");

                if (s.m == 0) {
                  $('#mode').attr('src', '/static/paused.png');
                }
                else {
                  $('#mode').attr('src', '/static/recording.png');
                }


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

              $('#ping').html((now.getTime() - client.getTime()).toString() + ' ms');
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
