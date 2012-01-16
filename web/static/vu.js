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

  if (l > -48 || (maxL >= -48 && maxL < -42))
    $('#l48').attr('src', '/static/green_on.png');
  else if (!(maxL >= -48 && maxL < -42))
    $('#l48').attr('src', '/static/black_led.png');

  if (l > -42 || (maxL >= -42 && maxL < -36))
    $('#l42').attr('src', '/static/green_on.png');
  else if (!(maxL >= -42 && maxL < -36))
    $('#l42').attr('src', '/static/black_led.png');

  if (l > -36 || (maxL >= -36 && maxL < -30))
    $('#l36').attr('src', '/static/green_on.png');
  else if (!(maxL >= -36 && maxL < -30))
    $('#l36').attr('src', '/static/black_led.png');

  if (l > -30 || (maxL >= -30 && maxL < -24))
    $('#l30').attr('src', '/static/green_on.png');
  else if (!(maxL >= -30 && maxL < -24))
    $('#l30').attr('src', '/static/black_led.png');

  if (l > -24 || (maxL >= -24 && maxL < -18))
    $('#l24').attr('src', '/static/green_on.png');
  else if (!(maxL >= -24 && maxL < -18))
  $('#l24').attr('src', '/static/black_led.png');

  if (l > -18 || (maxL >= -18 && maxL < -12))
    $('#l18').attr('src', '/static/green_on.png');
  else if (!(maxL >= -18 && maxL < -12))
    $('#l18').attr('src', '/static/black_led.png');

  if (l > -15 || (maxL >= -1 && maxL < -12))
    $('#l15').attr('src', '/static/green_on.png');
  else if (!(maxL >= -15 && maxL < -12))
    $('#l15').attr('src', '/static/black_led.png');

  if (l > -12 || (maxL >= -12 && maxL < -9))
    $('#l12').attr('src', '/static/green_on.png');
  else if (!(maxL >= -12 && maxL < -9))
    $('#l12').attr('src', '/static/black_led.png');

  if (l > -9 || (maxL >= -9 && maxL < -6))
    $('#l9').attr('src', '/static/green_on.png');
  else if (!(maxL >= -9 && maxL < -6))
    $('#l9').attr('src', '/static/black_led.png');

  if (l > -6 || (maxL >= -6 && maxL < -5))
    $('#l6').attr('src', '/static/green_on.png');
  else if (!(maxL >= -6 && maxL < -5))
    $('#l6').attr('src', '/static/black_led.png');

  if (l > -5 || (maxL >= -5 && maxL < -4))
    $('#l5').attr('src', '/static/green_on.png');
  else if (!(maxL >= -5 && maxL < -4))
    $('#l5').attr('src', '/static/black_led.png');

  if (l > -4 || (maxL >= -4 && maxL < -3))
    $('#l4').attr('src', '/static/green_on.png');
  else if (!(maxL >= -4 && maxL < -3))
    $('#l4').attr('src', '/static/black_led.png');

  if (l > -3 || (maxL >= -3 && maxL < -2))
    $('#l3').attr('src', '/static/green_on.png');
  else if (!(maxL >= -3 && maxL < -2))
    $('#l3').attr('src', '/static/black_led.png');

  if (l > -2 || (maxL >= -2 && maxL < -1))
    $('#l2').attr('src', '/static/green_on.png');
  else if (!(maxL >= -2 && maxL < -1))
    $('#l2').attr('src', '/static/black_led.png');

  if (l > -1 || (maxL >= -1 && maxL < 0))
    $('#l1').attr('src', '/static/amber_on.png');
  else if (!(maxL >= -1 && maxL < 0))
    $('#l1').attr('src', '/static/black_led.png');

  if (l == 0 || maxL == 0)
    $('#l0').attr('src', '/static/red_on.png');
  else if (maxL != 0)
    $('#l0').attr('src', '/static/black_led.png');


  // #### RIGHT

  if (r > -48 || (maxR >= -48 && maxR < -42))
    $('#r48').attr('src', '/static/green_on.png');
  else if (!(maxR >= -48 && maxR < -42))
    $('#r48').attr('src', '/static/black_led.png');

  if (r > -42 || (maxR >= -42 && maxR < -36))
    $('#r42').attr('src', '/static/green_on.png');
  else if (!(maxR >= -42 && maxR < -36))
    $('#r42').attr('src', '/static/black_led.png');

  if (r > -36 || (maxR >= -36 && maxR < -30))
    $('#r36').attr('src', '/static/green_on.png');
  else if (!(maxR >= -36 && maxR < -30))
    $('#r36').attr('src', '/static/black_led.png');

  if (r > -30 || (maxR >= -30 && maxR < -24))
    $('#r30').attr('src', '/static/green_on.png');
  else if (!(maxR >= -30 && maxR < -24))
    $('#r30').attr('src', '/static/black_led.png');

  if (r > -24 || (maxR >= -24 && maxR < -18))
    $('#r24').attr('src', '/static/green_on.png');
  else if (!(maxR >= -24 && maxR < -18))
  $('#r24').attr('src', '/static/black_led.png');

  if (r > -18 || (maxR >= -18 && maxR < -12))
    $('#r18').attr('src', '/static/green_on.png');
  else if (!(maxR >= -18 && maxR < -12))
    $('#r18').attr('src', '/static/black_led.png');

  if (l > -15 || (maxR >= -1 && maxR < -12))
    $('#r15').attr('src', '/static/green_on.png');
  else if (!(maxR >= -15 && maxR < -12))
    $('#r15').attr('src', '/static/black_led.png');

  if (l > -12 || (maxR >= -12 && maxR < -9))
    $('#r12').attr('src', '/static/green_on.png');
  else if (!(maxR >= -12 && maxR < -9))
    $('#r12').attr('src', '/static/black_led.png');

  if (l > -9 || (maxR >= -9 && maxR < -6))
    $('#r9').attr('src', '/static/green_on.png');
  else if (!(maxR >= -9 && maxR < -6))
    $('#r9').attr('src', '/static/black_led.png');

  if (r > -6 || (maxR >= -6 && maxR < -5))
    $('#r6').attr('src', '/static/green_on.png');
  else if (!(maxR >= -6 && maxR < -5))
    $('#r6').attr('src', '/static/black_led.png');

  if (r > -5 || (maxR >= -5 && maxR < -4))
    $('#r5').attr('src', '/static/green_on.png');
  else if (!(maxR >= -5 && maxR < -4))
    $('#r5').attr('src', '/static/black_led.png');

  if (r > -4 || (maxR >= -4 && maxR < -3))
    $('#r4').attr('src', '/static/green_on.png');
  else if (!(maxR >= -4 && maxR < -3))
    $('#r4').attr('src', '/static/black_led.png');

  if (r > -3 || (maxR >= -3 && maxR < -2))
    $('#r3').attr('src', '/static/green_on.png');
  else if (!(maxR >= -3 && maxR < -2))
    $('#r3').attr('src', '/static/black_led.png');

  if (r > -2 || (maxR >= -2 && maxR < -1))
    $('#r2').attr('src', '/static/green_on.png');
  else if (!(maxR >= -2 && maxR < -1))
    $('#r2').attr('src', '/static/black_led.png');

  if (r > -1 || (maxR >= -1 && maxR < 0))
    $('#r1').attr('src', '/static/amber_on.png');
  else if (!(maxR >= -1 && maxR < 0))
    $('#r1').attr('src', '/static/black_led.png');

  if (r == 0 || maxR == 0)
    $('#r0').attr('src', '/static/red_on.png');
  else if (maxR != 0)
    $('#r0').attr('src', '/static/black_led.png');

}
