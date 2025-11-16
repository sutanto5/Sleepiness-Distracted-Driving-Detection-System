import Toybox.Lang;
import Toybox.WatchUi;

class garmin55Delegate extends WatchUi.BehaviorDelegate {

    function initialize() {
        BehaviorDelegate.initialize();
    }

    function onMenu() as Boolean {
        WatchUi.pushView(new Rez.Menus.MainMenu(), new garmin55MenuDelegate(), WatchUi.SLIDE_UP);
        return true;
    }

}