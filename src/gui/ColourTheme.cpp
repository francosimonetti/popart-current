#include "ColourTheme.h"

#include <vector>
using namespace std;

QVector<QColor> initTheme(QColor *array, int ncols)
{
  vector<QColor> tmp;
  tmp.assign(array, array + ncols);

  QVector<QColor> vect = QVector<QColor>::fromStdVector(tmp);

  return vect;
}

#define NCOLS 10
#define NCOLSF 31

QColor greyarray[NCOLS] = {Qt::white, QColor(204, 204, 204), Qt::black, QColor(249, 249, 249), QColor(26, 26, 26),
                              QColor(242, 242, 242), QColor(179, 179, 179),QColor(51, 51, 51), QColor(230, 230, 230),
                              QColor(102, 102, 102)};
const QVector<QColor> ColourTheme::_greyscale = initTheme(greyarray, NCOLS);
    /*initTheme((QColor[NCOLS]){Qt::white, QColor(204, 204, 204), Qt::black, QColor(249, 249, 249), QColor(26, 26, 26),
                              QColor(242, 242, 242), QColor(179, 179, 179),QColor(51, 51, 51), QColor(230, 230, 230),
                              QColor(102, 102, 102)}, NCOLS);*/

QColor camoarray[NCOLS] = {QColor(80, 45, 22), QColor(51, 128, 0), QColor(77, 77, 77), QColor(188, 211, 95),
                              QColor(22, 80, 22), QColor(200, 190, 183), QColor(31, 36, 28), QColor(136, 170, 0),
                              QColor(172, 147, 147), QColor(83, 108, 83)};
const QVector<QColor> ColourTheme::_camo = initTheme(camoarray, NCOLS);
    /*initTheme((QColor[NCOLS]){QColor(80, 45, 22), QColor(51, 128, 0), QColor(77, 77, 77), QColor(188, 211, 95),
                              QColor(22, 80, 22), QColor(200, 190, 183), QColor(31, 36, 28), QColor(136, 170, 0),
                              QColor(172, 147, 147), QColor(83, 108, 83)}, NCOLS);*/

QColor pastellearray[NCOLS] = {QColor(175, 198, 233), QColor(170, 255, 170), QColor(229, 128, 255), QColor(255, 238, 170),
                              QColor(255, 128, 229), QColor(255, 170, 170), QColor(198, 175, 233), QColor(233, 175, 198),
                              QColor(230, 255, 180), QColor(100, 255, 255)};
const QVector<QColor> ColourTheme::_pastelle = initTheme(pastellearray, NCOLS);
    /*initTheme((QColor[NCOLS]){QColor(175, 198, 233), QColor(170, 255, 170), QColor(229, 128, 255), QColor(255, 238, 170),
                              QColor(255, 128, 229), QColor(255, 170, 170), QColor(198, 175, 233), QColor(233, 175, 198),
                              QColor(230, 255, 180), QColor(100, 255, 255)}, NCOLS);*/

QColor vibrantarray[NCOLS] = {QColor(255, 0, 0), QColor(55, 200, 55), QColor(102, 0, 128), QColor(255, 204, 0),
                              QColor(255, 0, 204), QColor(170, 0, 0), QColor(85, 0, 212), QColor(255, 0, 102),
                              QColor(215, 255, 42), QColor(0, 204, 255)};
const QVector<QColor> ColourTheme::_vibrant = initTheme(vibrantarray, NCOLS);
    /*initTheme((QColor[NCOLS]){QColor(255, 0, 0), QColor(55, 200, 55), QColor(102, 0, 128), QColor(255, 204, 0),
                              QColor(255, 0, 204), QColor(170, 0, 0), QColor(85, 0, 212), QColor(255, 0, 102),
                              QColor(215, 255, 42), QColor(0, 204, 255)}, NCOLS);*/

QColor springarray[NCOLS] =  {QColor(255, 85, 85), QColor(221, 255, 85), QColor(229, 128, 255), QColor(128, 179, 255),
                              QColor(255, 0, 204), QColor(0, 255, 102), QColor(198, 175, 233), QColor(255, 85, 153),
                              QColor(55, 200, 171), QColor(102, 255, 255)};
const QVector<QColor> ColourTheme::_spring = initTheme(springarray, NCOLS);
    /*initTheme((QColor[NCOLS]){QColor(255, 85, 85), QColor(221, 255, 85), QColor(229, 128, 255), QColor(128, 179, 255),
                              QColor(255, 0, 204), QColor(0, 255, 102), QColor(198, 175, 233), QColor(255, 85, 153),
                              QColor(55, 200, 171), QColor(102, 255, 255)}, NCOLS); */

QColor summerarray[NCOLS] = {QColor(0, 212, 0), QColor(255, 42, 127), QColor(0, 128, 102), QColor(255, 102, 0),
                              QColor(0, 102, 255), QColor(212, 0, 170), QColor(255, 85, 85), QColor(255, 204, 0),
                              QColor(171, 55, 200), QColor(192, 0, 0)};
const QVector<QColor> ColourTheme::_summer = initTheme(summerarray, NCOLS);
    /*initTheme((QColor[NCOLS]){QColor(0, 212, 0), QColor(255, 42, 127), QColor(0, 128, 102), QColor(255, 102, 0),
                              QColor(0, 102, 255), QColor(212, 0, 170), QColor(255, 85, 85), QColor(255, 204, 0),
                              QColor(171, 55, 200), QColor(192, 0, 0)}, NCOLS);*/

QColor autumnarray[NCOLS] = {QColor(51, 51, 51), QColor(128, 0, 0), QColor(255, 104, 0), Qt::red, QColor(255, 102, 0),
                              QColor(160, 190, 44), QColor(80, 45, 22), QColor(22, 80, 22), QColor(200, 55, 55),
                              QColor(200, 171, 55)};
const QVector<QColor> ColourTheme::_autumn = initTheme(autumnarray, NCOLS);
    /*initTheme((QColor[NCOLS]){QColor(51, 51, 51), QColor(128, 0, 0), QColor(255, 104, 0), Qt::red, QColor(255, 102, 0),
                              QColor(160, 190, 44), QColor(80, 45, 22), QColor(22, 80, 22), QColor(200, 55, 55),
                              QColor(200, 171, 55)}, NCOLS);*/

// QColor winterarray[NCOLS] = {QColor(128, 0, 0), QColor(0, 85, 68), QColor(51, 51, 51), QColor(198, 175, 233),
//                               QColor(22, 80, 22), Qt::black, QColor(102, 0, 128), Qt::white, QColor(51, 0, 128),
//                               QColor(100, 255, 255)};
// const QVector<QColor> ColourTheme::_winter = initTheme(winterarray, NCOLS);
    /*initTheme((QColor[NCOLS]){QColor(128, 0, 0), QColor(0, 85, 68), QColor(51, 51, 51), QColor(198, 175, 233),
                              QColor(22, 80, 22), Qt::black, QColor(102, 0, 128), Qt::white, QColor(51, 0, 128),
                              QColor(100, 255, 255)}, NCOLS);*/

QColor francoarray[NCOLSF] = {QColor(219, 19, 65), QColor(224, 4, 94), QColor(223, 19, 123), QColor(215, 42, 152), QColor(200, 64, 179), QColor(179, 83, 204), QColor(148, 101, 225), QColor(105, 117, 241), QColor(0, 130, 252), QColor(0, 150, 251), QColor(0, 165, 255), QColor(0, 178, 254), QColor(0, 189, 239), QColor(0, 197, 214), QColor(0, 204, 182), QColor(0, 209, 145), QColor(60, 212, 131), QColor(89, 215, 117), QColor(114, 217, 102), QColor(138, 218, 87), QColor(160, 219, 71), QColor(183, 219, 55), QColor(206, 218, 38), QColor(229, 216, 20), QColor(252, 213, 0), QColor(38, 38, 38), QColor(69, 69, 69), QColor(102, 102, 102), QColor(137, 137, 137), QColor(174, 174, 174), QColor(254, 254, 254)};
const QVector<QColor> ColourTheme::_franco = initTheme(francoarray, NCOLSF);

// QColor(219, 19, 65), 
// QColor(224, 4, 94), 
// QColor(223, 19, 123), 
// QColor(215, 42, 152), 
// QColor(200, 64, 179), 
// QColor(179, 83, 204), 
// QColor(148, 101, 225), 
// QColor(105, 117, 241), 
// QColor(0, 130, 252), 
// QColor(0, 150, 251), 
// QColor(0, 165, 255), 
// QColor(0, 178, 254), 
// QColor(0, 189, 239), 
// QColor(0, 197, 214), 
// QColor(0, 204, 182), 
// QColor(0, 209, 145), 
// QColor(60, 212, 131), 
// QColor(89, 215, 117), 
// QColor(114, 217, 102), 
// QColor(138, 218, 87), 
// QColor(160, 219, 71), 
// QColor(183, 219, 55), 
// QColor(206, 218, 38), 
// QColor(229, 216, 20), 
// QColor(252, 213, 0), 

// QColor(213, 213, 212), 
// QColor(174, 174, 174), 
// QColor(137, 137, 137), 
// QColor(102, 102, 102), 
// QColor(69, 69, 69), 
// QColor(38, 38, 38)

// QColor(38, 38, 38), QColor(69, 69, 69), QColor(102, 102, 102), QColor(137, 137, 137), QColor(174, 174, 174), QColor(213, 213, 212)

// rgb(219, 19, 65), 
// rgb(224, 4, 94), 
// rgb(223, 19, 123), 
// rgb(215, 42, 152), 
// rgb(200, 64, 179), 
// rgb(179, 83, 204), 
// rgb(148, 101, 225), 
// rgb(105, 117, 241), 
// rgb(0, 130, 252), 
// rgb(0, 150, 251), 
// rgb(0, 165, 255), 
// rgb(0, 178, 254), 
// rgb(0, 189, 239), 
// rgb(0, 197, 214), 
// rgb(0, 204, 182), 
// rgb(0, 209, 145), 
// rgb(60, 212, 131), 
// rgb(89, 215, 117), 
// rgb(114, 217, 102), 
// rgb(138, 218, 87), 
// rgb(160, 219, 71), 
// rgb(183, 219, 55), 
// rgb(206, 218, 38), 
// rgb(229, 216, 20), 
// rgb(252, 213, 0), 


// #db1341
// #e0055e
// #de147b
// #d72b98
// #c840b3
// #b254cc
// #9466e1
// #6975f1
// #0082fc
// #0093ff
// #00a1ff
// #00adfb
// #00b7ec
// #00bfd7
// #00c6c0
// #00cca8
// #00d191
// #3cd483
// #5ad775
// #73d966
// #8ada57
// #a1db47
// #b7db37
// #ceda26
// #e5d814
// #fcd500