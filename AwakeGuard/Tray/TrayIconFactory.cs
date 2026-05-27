using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace AwakeGuard.Tray;

internal static class TrayIconFactory
{
    public static ImageSource Create(bool active)
    {
        const int size = 32;

        var fill = new SolidColorBrush(
            active
                ? Color.FromRgb(16, 185, 129)
                : Color.FromRgb(120, 120, 120));
        fill.Freeze();

        var visual = new DrawingVisual();
        using (var context = visual.RenderOpen())
        {
            context.DrawEllipse(fill, null, new Point(size / 2.0, size / 2.0), 14, 14);
        }

        var bitmap = new RenderTargetBitmap(size, size, 96, 96, PixelFormats.Pbgra32);
        bitmap.Render(visual);
        bitmap.Freeze();
        return bitmap;
    }
}
