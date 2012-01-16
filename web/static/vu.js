var maxL = -96;
var maxR = -96;

function resetVUMax() {
  maxL = maxR = -96;
}

function lightVU(peaks) {
  var l = Math.round(peaks[0]);
  var r = Math.round(peaks[1]);

  if(l > maxL)
    maxL = l;
  if(r > maxR)
    maxR = r;

  var leds = [-48, -42, -36, -30, -24, -18, -15, -12, -9, -6, -5, -4, -3, -2, -1, 0];
  var green = '/static/green_on.png';
  var amber = '/static/amber_on.png';
  var black = '/static/black_led.png';
  var red = '/static/red_on.png';

  var light = black;

  var i;
  for(i = 0;i < leds.length - 1;i++) {
    led = leds[i];

    if (l > led || (maxL >= led && maxL < leds[i+1])) {
      light = green;
      if(led == -1)
        light = amber;
      $("#l" + Math.abs(led)).attr('src', light);
    }
    else if (!(maxL >= led && maxL < leds[i+1]))
      $("#l" + Math.abs(led)).attr('src', black);

    if (r > led || (maxR >= led && maxR < leds[i+1])) {
      light = green;
      if(led == -1)
        light = amber;
      $("#r" + Math.abs(led)).attr('src', light);
    }
    else if (!(maxR >= led && maxR < leds[i+1]))
      $("#r" + Math.abs(led)).attr('src', black);

  }

  if (l == 0 || maxL == 0)
    $('#l0').attr('src', red);
  else if (maxL != 0)
    $('#l0').attr('src', black);

  if (r == 0 || maxR == 0)
    $('#r0').attr('src', red);
  else if (maxR != 0)
    $('#r0').attr('src', black);

}
