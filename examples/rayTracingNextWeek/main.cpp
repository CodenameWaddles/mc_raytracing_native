#include "app.h"

int main()
{
    pgSetAppDir(APP_DIR);
    pgSetAppName(APP_NAME_DEFINE);

    auto window = make_shared<Window>("Ray Tracing The Next Week", 1080, 1080);
    auto app = make_shared<App>();

    pgRunApp(app, window);

    return 0;
}