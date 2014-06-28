var app = angular.module('James', [
    'bd.sockjs',
    'mgcrea.ngStrap',
])
    .factory('sock', function (socketFactory) {
        return socketFactory({url:'http://' + window.location.host, debug:true});
    });

function RecordCtrl ($scope, sock, $http) {

    $scope.resetPeaks = function() {
        $http.post("/resetPeaks", {});
    }

    $scope.record = function() {
        $http.post("/record", {});
    }

    sock.setHandler('open', function(data) {
        function sendPing()
        {
            sock.send(JSON.stringify({client: encodeDate(new Date()) }));
            setTimeout(sendPing, 5000);
        }

        sendPing();
    });

    sock.setHandler('message', function(data) {
        var s = data.data;
        if (s._t === "peaks") {
            lightVU($scope, s.p);
            $scope.maxL = s.p[2];
            $scope.maxR = s.p[3];
        }
        else if (s._t === "status") {
            var allLeft = s.r / 3600;
            var hoursLeft = Math.floor(allLeft);
            var minutesLeft = Math.floor((allLeft - hoursLeft) * 60);
            $scope.timeLeft = hoursLeft + ":" + zeroPad(minutesLeft, 2);

            var elapsed = s.t / s.sr / 3600;
            var hoursElapsed = Math.floor(elapsed);
            var minutes = (elapsed - hoursElapsed) * 60;
            var minutesElapsed = Math.floor(minutes);
            var secondsElapsed = Math.floor((minutes - minutesElapsed) * 60);
            $scope.timeElapsed  = zeroPad(hoursElapsed, 2) + ":" +
                                   zeroPad(minutesElapsed, 2) + ":" +
                                   zeroPad(secondsElapsed, 2);

            $scope.format = s.ss.toString() + "/" + s.sr.toString().slice(0,2);

            $scope.buffer = s.b + "s";

            //$('#load').html("(" + s.c[0] + ", " + s.c[1] + ", " + s.c[2] + ")");
            $scope.temp = s.ct + "&deg;";

            $scope.signal = s.s ? "LOCKED" : "NO SIGNAL";

            if (s.m == 0) {
                $scope.mode = '/static/paused.png';
            }
            else {
                $scope.mode = '/static/recording.png';
            }

        }
        else if (s._t === "pong")  {
            var client = decodeDate(s.client);
            var server = decodeDate(s.server);
            var now = new Date();

            $scope.ping = (now.getTime() - client.getTime()).toString() + ' ms';
        }
    })
};

function zeroPad(num,count)
{
    var numZeropad = num + '';
    while(numZeropad.length < count) {
        numZeropad = "0" + numZeropad;
    }
    return numZeropad;
}

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
