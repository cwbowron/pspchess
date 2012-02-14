/* unsigned short rgb2col(unsigned char r,unsigned char g,unsigned char b){return ((((b>>3) & 0x1F)<<10)+(((g>>3) & 0x1F)<<5)+(((r>>3) & 0x1F)<<0)+0x8000);} */

#define rgb2col(r,g,b) ((((b>>3) & 0x1F)<<10)+(((g>>3) & 0x1F)<<5)+(((r>>3) & 0x1F)<<0)+0x8000)

#define aliceblue rgb2col(240,248,255)
#define antiquewhite rgb2col(250,235,215)
#define aqua rgb2col(0,255,255)
#define aquamarine rgb2col(127,255,212)
#define azure rgb2col(240,255,255)
#define beige rgb2col(245,245,220)
#define bisque rgb2col(255,228,196)
#define black rgb2col(0,0,0)
#define blanchedalmond rgb2col(255,235,205)
#define blue rgb2col(0,0,255)
#define blueviolet rgb2col(138,43,226)
#define brown rgb2col(165,42,42)
#define burlywood rgb2col(222,184,135)
#define cadetblue rgb2col(95,158,160)
#define charteuse rgb2col(127,255,0)
#define chocolate rgb2col(210,105,30)
#define coral rgb2col(255,127,80)
#define cornflowerblue rgb2col(100,149,237)
#define cornsilk rgb2col(255,248,220)
#define crimson	rgb2col(220,20,60)
#define cyan rgb2col(0,255,255)
#define darkblue rgb2col(0,0,139)
#define darkcyan rgb2col(0,139,139)
#define darkgoldenrod rgb2col(184,134,11)
#define darkgray rgb2col(169,169,169)
#define darkgreen rgb2col(0,100,0)
#define darkkhaki rgb2col(189,183,107)
#define darkmagenta rgb2col(139,0,139)
#define darkolivegreen rgb2col(85,107,47)
#define darkorange rgb2col(255,140,0)
#define darkorchid rgb2col(153,50,204)
#define darkred	rgb2col(139,0,0)
#define darksalmon rgb2col(233,150,122)
#define darkseagreen rgb2col(143,188,143)
#define darkslateblue rgb2col(72,61,139)
#define darkslategray rgb2col(47,79,79)
#define darkturquoise rgb2col(0,206,209)
#define darkviolet rgb2col(148,0,211)
#define deeppink rgb2col(255,20,147)
#define deepskyblue rgb2col(0,191,255)
#define dimgray rgb2col(105,105,105)
#define dodgerblue rgb2col(30,144,255)
#define firebrick rgb2col(178,34,34)
#define floralwhite rgb2col(255,250,240)
#define forestgreen rgb2col(34,139,34)
#define fuchsia rgb2col(255,0,255)
#define gainsboro rgb2col(220,220,220)
#define ghostwhite rgb2col(248,248,255)
#define gold rgb2col(255,215,0)
#define goldenrod rgb2col(218,165,32)
#define gray rgb2col(128,128,128)
#define green rgb2col(0,128,0)
#define greenyellow rgb2col(173,255,47)
#define honeydew rgb2col(240,255,240)
#define hotpink rgb2col(255,105,180)
#define indianred rgb2col(205,92,92)
#define indigo rgb2col(75,0,130)
#define ivory rgb2col(255,255,240)
#define khaki rgb2col(240,230,140)
#define lavender rgb2col(230,230,250)
#define lavenderblush rgb2col(255,240,245)
#define lawngreen rgb2col(124,252,0)
#define lemonchiffon rgb2col(255,250,205)
#define lightblue rgb2col(173,216,230)
#define lightcoral rgb2col(240,128,128)
#define lightcyan rgb2col(224,255,255)
#define lightgoldenrodyellow rgb2col(250,250,210)
#define lightgreen rgb2col(144,238,144)
#define lightgrey rgb2col(211,211,211)
#define lightpink rgb2col(255,182,193)
#define lightsalmon rgb2col(255,160,122)
#define lightseagreen rgb2col(32,178,170)
#define lightskyblue rgb2col(135,205,250)
#define lightslategray rgb2col(119,136,153)
#define lightsteelblue rgb2col(176,196,222)
#define lightyellow rgb2col(255,255,224)
#define lime rgb2col(0,255,0)
#define limegreen rgb2col(50,205,50)
#define linen rgb2col(135,240,230)
#define magenta rgb2col(255,0,255)
#define maroon rgb2col(128,0,0)
#define mediumaquamarine rgb2col(102,205,170)
#define mediumblue rgb2col(0,0,205)
#define mediumorchid rgb2col(186,85,211)
#define mediumpurple rgb2col(147,112,219)
#define mediumseagreen rgb2col(60,179,113)
#define mediumslateblue rgb2col(123,104,238)
#define mediumspringgreen rgb2col(0,135,154)
#define mediumturquoise rgb2col(72,209,204)
#define mediumvioletred rgb2col(199,21,133)
#define midnightblue rgb2col(25,25,112)
#define mintcream rgb2col(245,255,135)
#define mistyrose rgb2col(255,228,225)
#define navajowhite rgb2col(255,222,173)
#define navy rgb2col(0,0,128)
#define oldlace rgb2col(153,245,246)
#define olive rgb2col(128,128,0)
#define olivedrab rgb2col(107,142,35)
#define orange rgb2col(255,165,0)
#define orangered rgb2col(255,69,0)
#define orchid rgb2col(218,112,214)
#define palegoldenrod rgb2col(238,232,170)
#define palegreen rgb2col(152,251,152)
#define paleturquoise rgb2col(175,138,138)
#define palevioletred rgb2col(219,112,147)
#define papaawhip rgb2col(255,239,213)
#define peachpuff rgb2col(255,218,185)
#define peru rgb2col(205,133,63)
#define pink rgb2col(255,192,203)
#define plum rgb2col(221,160,221)
#define powderblue rgb2col(176,224,230)
#define purple rgb2col(128,0,128)
#define red rgb2col(255,0,0)
#define rosybrown rgb2col(188,143,143)
#define royalblue rgb2col(65,105,225)
#define saddlebrown rgb2col(139,69,19)
#define salmon rgb2col(135,128,114)
#define sandybrown rgb2col(244,164,96)
#define seagreen rgb2col(46,139,87)
#define seashell rgb2col(255,245,238)
#define sienna rgb2col(160,82,45)
#define silver rgb2col(192,192,192)
#define skyblue rgb2col(135,206,235)
#define slateblue rgb2col(106,90,205)
#define snow rgb2col(255,250,250)
#define springgreen rgb2col(0,255,127)
#define steelblue rgb2col(70,130,180)
#define tan rgb2col(210,180,140)
#define teal rgb2col(0,128,128)
#define thistle rgb2col(216,191,216)
#define tomato rgb2col(255,99,71)
#define turquoise rgb2col(64,224,208)
#define violet rgb2col(238,130,238)
#define wheat rgb2col(245,222,179)
#define white rgb2col(255,255,255)
#define whitesmoke rgb2col(245,245,245)
#define yellow rgb2col(255,255,0)
#define yellowgreen rgb2col(154,205,50)

