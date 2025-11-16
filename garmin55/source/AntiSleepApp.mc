using Toybox.Application as App;
using Toybox.Lang;
using Toybox.WatchUi as Ui;

class AntiSleepApp extends App.AppBase {

    function initialize() {
        App.AppBase.initialize();
    }

    function onStart(state) {
        // Nothing special here yet.
    }

    function onStop(state) {
        // Cleanup handled in the view.
    }

    function getInitialView() {
        return [ new AntiSleepView() ];
    }
}
