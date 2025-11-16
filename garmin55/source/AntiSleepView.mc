using Toybox.WatchUi as Ui;
using Toybox.Graphics as Gfx;
using Toybox.Sensor as Sensor;
using Toybox.Attention as Attention;
using Toybox.Timer as Timer;
using Toybox.Time as Time;

class AntiSleepView extends Ui.View {

    // === TUNABLE SETTINGS ===
    const HR_THRESHOLD          = 52;           // bpm – adjust to taste
    const LOW_HR_DURATION_SEC   = 5;           // how long HR must stay low
    const SAMPLE_INTERVAL_MS    = 1000;         // 1 second between checks

    // === STATE ===
    var _timer;
    var _currentHr      = null; // latest HR value
    var _isLow          = false;
    var _lowSince       = null; // timestamp when HR first went below threshold
    var _alerted        = false; // to avoid constant re-vibration
    var _alertMessage        = null;
    var _alertShownUntilSec  = null;

    function initialize() {
        View.initialize();
    }

    function onShow() {
        // Enable HR sensor
        Sensor.setEnabledSensors([Sensor.SENSOR_HEARTRATE]);

        // Start periodic timer
        _timer = new Timer.Timer();
        _timer.start(method(:onSample), SAMPLE_INTERVAL_MS, true);
    }

    function onHide() {
        // Stop timer
        if (_timer != null) {
            _timer.stop();
            _timer = null;
        }

        // Disable sensors to save battery
        Sensor.setEnabledSensors([]);
    }

    // Called every SAMPLE_INTERVAL_MS
    function onSample() as Void {
        var info = Sensor.getInfo();

        if (info != null && info.heartRate != null) {
            _currentHr = info.heartRate;
            checkForSleepiness();
        }

        // Redraw UI to show latest HR
        Ui.requestUpdate();
    }

    // Core “sleepiness” heuristic
    function checkForSleepiness() {
        if (_currentHr == null) {
            _isLow    = false;
            _lowSince = null;
            return;
        }

        var now = Time.now();

        if (_currentHr < HR_THRESHOLD) {
            if (!_isLow) {
                // Just dropped below threshold
                _isLow    = true;
                _lowSince = now;
            } else {
                // Stayed low for some time
                if (!_alerted && _lowSince != null &&
                    (now - _lowSince) >= LOW_HR_DURATION_SEC) {

                    // We consider this a “you might be dozing off” event
                    tryVibrate();
                    _alerted = true;
                }
            }
        } else { // HR back above threshold
            _isLow       = false;
            _lowSince = null;
            _alerted     = false;

            // Optionally clear alert display too
            _alertMessage       = null;
            _alertShownUntilSec = null;
        }
    }

    // Vibrate once (if vibration is available/enabled on device)
    function tryVibrate() {
        // Turn on / brighten backlight
        if (Attention has :backlight) {
            try {
                // Try full brightness if supported
                Attention.backlight(1.0);   // max brightness
            } catch(e) {
                // Fallback to simple "on" if Float not supported
                Attention.backlight(true);
            }
        }

        // Vibrate
        if (Attention has :vibrate) {
            var pattern = [ new Attention.VibeProfile(100, 800) ]; // 0.8s buzz
            Attention.vibrate(pattern);
        }

        // Tell the UI to show an alert message for a bit
        var nowSec = Time.now().value();
        _alertMessage      = "HR low – stay awake!";
        _alertShownUntilSec = nowSec +7;   // show message for 7 seconds
    }

    function onUpdate(dc as Gfx.Dc) {
        dc.clear();

        var w = dc.getWidth();
        var h = dc.getHeight();

        dc.setColor(Gfx.COLOR_WHITE, Gfx.COLOR_BLACK);

        // Title
        dc.drawText(w/2, h/6, Gfx.FONT_MEDIUM, "AntiSleep HR",
                    Gfx.TEXT_JUSTIFY_CENTER);

        // HR display
        var hrText = (_currentHr == null)
            ? "HR: --"
            : "HR: " + _currentHr.toString() + " bpm";

        dc.drawText(w/2, h*0.4, Gfx.FONT_LARGE, hrText,
                    Gfx.TEXT_JUSTIFY_CENTER);

        // Threshold info
        var thText = "Threshold: " + HR_THRESHOLD.toString() + " bpm";
        dc.drawText(w/2, h*0.57, Gfx.FONT_SMALL, thText,
                    Gfx.TEXT_JUSTIFY_CENTER);

        // Decide what status / message to show at the bottom
        var nowSec  = Time.now().value();
        var bottomText;

        if (_alertMessage != null && _alertShownUntilSec != null &&
            nowSec <= _alertShownUntilSec) {

            // During alert window: show big warning
            bottomText = _alertMessage;
        } else {
            // Normal status
            if (_currentHr == null) {
                bottomText = "Waiting for HR...";
            } else if (_alerted) {
                bottomText = "Alert triggered (HR low)";
            } else if (_isLow) {
                bottomText = "Low HR... monitoring";
            } else {
                bottomText = "Awake / normal HR";
            }
        }

        dc.drawText(w/2, h*0.69, Gfx.FONT_SMALL, bottomText,
                    Gfx.TEXT_JUSTIFY_CENTER);
    }

}
