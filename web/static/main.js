function zeroPad(num,count)
{
    var numZeropad = num + '';
    while(numZeropad.length < count) {
        numZeropad = "0" + numZeropad;
    }
    return numZeropad;
}

$(function() {
    var sock = new SockJS('http://' + window.location.host);

    sock.onopen = function(data) {
        sendPing();
    };

    sock.onmessage =  function(data) {
        var s = data.data;
        if (s._t === "peaks") {
            lightVU(s.p);
            $('#maxL').html(s.p[2]);
            $('#maxR').html(s.p[3]);
        }
        else if (s._t === "status") {
            var allLeft = s.r / 3600;
            var hoursLeft = Math.floor(allLeft);
            var minutesLeft = Math.floor((allLeft - hoursLeft) * 60);
            $('#timeLeft').html(hoursLeft + ":" + zeroPad(minutesLeft, 2));

            var elapsed = s.t / 3600;
            var hoursElapsed = Math.floor(elapsed);
            var minutes = (elapsed - hoursElapsed) * 60;
            var minutesElapsed = Math.floor(minutes);
            var secondsElapsed = Math.floor((minutes - minutesElapsed) * 60);
            $('#timeElapsed').html(zeroPad(hoursElapsed, 2) + ":" +
                                   zeroPad(minutesElapsed, 2) + ":" +
                                   zeroPad(secondsElapsed, 2));

            $('#format').html(s.f);

            $('#buffer').html(s.b + "s");

            //$('#load').html("(" + s.c[0] + ", " + s.c[1] + ", " + s.c[2] + ")");
            $('#temp').html(s.ct + "&deg;");

            $('#signal').html(s.s ? "LOCKED" : "NO SIGNAL");
            
            if (s.m == 0) {
                $('#mode').attr('src', '/static/paused.png');
            }
            else {
                $('#mode').attr('src', '/static/recording.png');
            }

        }
        else if (s._t === "pong")  {
            var client = decodeDate(s.client);
            var server = decodeDate(s.server);
            var now = new Date();

            $('#ping').html((now.getTime() - client.getTime()).toString() + ' ms');
        }
    };


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

    function sendPing()
    {
        sock.send(JSON.stringify({client: encodeDate(new Date()) }));
        setTimeout(sendPing, 5000);
    }
});
