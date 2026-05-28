using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace AwakeGuard.Tray;

internal static class TrayIconFactory
{
    private static readonly Uri ActiveUri = new("pack://application:,,,/Assets/tray-active-32.png");
    private static readonly Uri InactiveUri = new("pack://application:,,,/Assets/tray-inactive-32.png");

    public static ImageSource Create(bool active)
    {
        var uri = active ? ActiveUri : InactiveUri;
        var decoder = BitmapDecoder.Create(
            uri,
            BitmapCreateOptions.None,
            BitmapCacheOption.OnLoad);

        var frame = decoder.Frames[0];
        frame.Freeze();
        return frame;
    }
}
