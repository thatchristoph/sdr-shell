#include <qapplication.h>
#include <qfontdatabase.h>
#include "main_widget.h"
#include "app.xpm"

//int DDS::loLcdValue=0;

int main (int argc, char **argv)
{
    QApplication app(argc, argv);
    Main_Widget *w = new Main_Widget;; 
    app.setWindowIcon( QIcon(app_xpm) );
    //app.setStyleSheet("QFrame {border : 1px solid rgb(0,0,0)}"); 
    w->show();
    return app.exec();
}
