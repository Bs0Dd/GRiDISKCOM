#include "mainwindow.h"

#include <QApplication>
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
//    uint8_t* dat = NULL;
//    size_t siz = 0;
//    read_file("CCOS310.IMG", &dat, &siz);
//    ccos_inode_t* root = ccos_get_root_dir(dat, siz);
//    uint16_t fils = 0;
//    ccos_inode_t** dirdata = NULL;
//    ccos_get_dir_contents(root, dat, &fils, &dirdata);
//    cout << fils << endl;
//    for(int c = 0; c < fils; c++){
//        cout << dirdata[c] << endl;
//    }
//    ccos_get_dir_contents(dirdata[0], dat, &fils, &dirdata);
//    cout << fils << endl;
//    for(int c = 0; c < fils; c++){
//        cout << dirdata[c] << endl;
//    }
//    char basename[CCOS_MAX_FILE_NAME];
//    char type[CCOS_MAX_FILE_NAME];
//    for(int c = 0; c < fils; c++){
//        memset(basename, 0, CCOS_MAX_FILE_NAME);
//        memset(type, 0, CCOS_MAX_FILE_NAME);
//        ccos_parse_file_name(dirdata[c], basename, type, NULL, NULL);
//        cout << basename << " & " << type << endl;
//    }
    //print_image_info("GRIDOS.IMG", dat, siz, 1);
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
