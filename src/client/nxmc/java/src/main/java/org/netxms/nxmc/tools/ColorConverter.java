/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.nxmc.tools;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.preference.IPreferenceStore;
import org.eclipse.jface.preference.PreferenceConverter;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.RGB;

/**
 * Utility class for converting between different color representation formats
 *
 */
public class ColorConverter
{
   private static final Map<String, RGB> colorNames;
   private static final Map<RGB, String> colorValues;

   static
   {
      colorNames = new HashMap<String, RGB>();
      colorNames.put("aliceblue", new RGB(0xf0, 0xf8, 0xff));
      colorNames.put("antiquewhite", new RGB(0xfa, 0xeb, 0xd7));
      colorNames.put("aqua", new RGB(0x00, 0xff, 0xff));
      colorNames.put("aquamarine", new RGB(0x7f, 0xff, 0xd4));
      colorNames.put("azure", new RGB(0xf0, 0xff, 0xff));
      colorNames.put("beige", new RGB(0xf5, 0xf5, 0xdc));
      colorNames.put("bisque", new RGB(0xff, 0xe4, 0xc4));
      colorNames.put("black", new RGB(0x00, 0x00, 0x00));
      colorNames.put("blanchedalmond", new RGB(0xff, 0xeb, 0xcd));
      colorNames.put("blue", new RGB(0x00, 0x00, 0xff));
      colorNames.put("blueviolet", new RGB(0x8a, 0x2b, 0xe2));
      colorNames.put("brown", new RGB(0xa5, 0x2a, 0x2a));
      colorNames.put("burlywood", new RGB(0xde, 0xb8, 0x87));
      colorNames.put("cadetblue", new RGB(0x5f, 0x9e, 0xa0));
      colorNames.put("chartreuse", new RGB(0x7f, 0xff, 0x00));
      colorNames.put("chocolate", new RGB(0xd2, 0x69, 0x1e));
      colorNames.put("coral", new RGB(0xff, 0x7f, 0x50));
      colorNames.put("cornflowerblue", new RGB(0x64, 0x95, 0xed));
      colorNames.put("cornsilk", new RGB(0xff, 0xf8, 0xdc));
      colorNames.put("crimson", new RGB(0xdc, 0x14, 0x3c));
      colorNames.put("cyan", new RGB(0x00, 0xff, 0xff));
      colorNames.put("darkblue", new RGB(0x00, 0x00, 0x8b));
      colorNames.put("darkcyan", new RGB(0x00, 0x8b, 0x8b));
      colorNames.put("darkgoldenrod", new RGB(0xb8, 0x86, 0x0b));
      colorNames.put("darkgray", new RGB(0xa9, 0xa9, 0xa9));
      colorNames.put("darkgrey", new RGB(0xa9, 0xa9, 0xa9));
      colorNames.put("darkgreen", new RGB(0x00, 0x64, 0x00));
      colorNames.put("darkkhaki", new RGB(0xbd, 0xb7, 0x6b));
      colorNames.put("darkmagenta", new RGB(0x8b, 0x00, 0x8b));
      colorNames.put("darkolivegreen", new RGB(0x55, 0x6b, 0x2f));
      colorNames.put("darkorange", new RGB(0xff, 0x8c, 0x00));
      colorNames.put("darkorchid", new RGB(0x99, 0x32, 0xcc));
      colorNames.put("darkred", new RGB(0x8b, 0x00, 0x00));
      colorNames.put("darksalmon", new RGB(0xe9, 0x96, 0x7a));
      colorNames.put("darkseagreen", new RGB(0x8f, 0xbc, 0x8f));
      colorNames.put("darkslateblue", new RGB(0x48, 0x3d, 0x8b));
      colorNames.put("darkslategray", new RGB(0x2f, 0x4f, 0x4f));
      colorNames.put("darkslategrey", new RGB(0x2f, 0x4f, 0x4f));
      colorNames.put("darkturquoise", new RGB(0x00, 0xce, 0xd1));
      colorNames.put("darkviolet", new RGB(0x94, 0x00, 0xd3));
      colorNames.put("deeppink", new RGB(0xff, 0x14, 0x93));
      colorNames.put("deepskyblue", new RGB(0x00, 0xbf, 0xff));
      colorNames.put("dimgray", new RGB(0x69, 0x69, 0x69));
      colorNames.put("dimgrey", new RGB(0x69, 0x69, 0x69));
      colorNames.put("dodgerblue", new RGB(0x1e, 0x90, 0xff));
      colorNames.put("firebrick", new RGB(0xb2, 0x22, 0x22));
      colorNames.put("floralwhite", new RGB(0xff, 0xfa, 0xf0));
      colorNames.put("forestgreen", new RGB(0x22, 0x8b, 0x22));
      colorNames.put("fuchsia", new RGB(0xff, 0x00, 0xff));
      colorNames.put("gainsboro", new RGB(0xdc, 0xdc, 0xdc));
      colorNames.put("ghostwhite", new RGB(0xf8, 0xf8, 0xff));
      colorNames.put("gold", new RGB(0xff, 0xd7, 0x00));
      colorNames.put("goldenrod", new RGB(0xda, 0xa5, 0x20));
      colorNames.put("gray", new RGB(0x80, 0x80, 0x80));
      colorNames.put("grey", new RGB(0x80, 0x80, 0x80));
      colorNames.put("green", new RGB(0x00, 0x80, 0x00));
      colorNames.put("greenyellow", new RGB(0xad, 0xff, 0x2f));
      colorNames.put("honeydew", new RGB(0xf0, 0xff, 0xf0));
      colorNames.put("hotpink", new RGB(0xff, 0x69, 0xb4));
      colorNames.put("indianred", new RGB(0xcd, 0x5c, 0x5c));
      colorNames.put("indigo", new RGB(0x4b, 0x00, 0x82));
      colorNames.put("ivory", new RGB(0xff, 0xff, 0xf0));
      colorNames.put("khaki", new RGB(0xf0, 0xe6, 0x8c));
      colorNames.put("lavender", new RGB(0xe6, 0xe6, 0xfa));
      colorNames.put("lavenderblush", new RGB(0xff, 0xf0, 0xf5));
      colorNames.put("lawngreen", new RGB(0x7c, 0xfc, 0x00));
      colorNames.put("lemonchiffon", new RGB(0xff, 0xfa, 0xcd));
      colorNames.put("lightblue", new RGB(0xad, 0xd8, 0xe6));
      colorNames.put("lightcoral", new RGB(0xf0, 0x80, 0x80));
      colorNames.put("lightcyan", new RGB(0xe0, 0xff, 0xff));
      colorNames.put("lightgoldenrodyellow", new RGB(0xfa, 0xfa, 0xd2));
      colorNames.put("lightgray", new RGB(0xd3, 0xd3, 0xd3));
      colorNames.put("lightgrey", new RGB(0xd3, 0xd3, 0xd3));
      colorNames.put("lightgreen", new RGB(0x90, 0xee, 0x90));
      colorNames.put("lightpink", new RGB(0xff, 0xb6, 0xc1));
      colorNames.put("lightsalmon", new RGB(0xff, 0xa0, 0x7a));
      colorNames.put("lightseagreen", new RGB(0x20, 0xb2, 0xaa));
      colorNames.put("lightskyblue", new RGB(0x87, 0xce, 0xfa));
      colorNames.put("lightslategray", new RGB(0x77, 0x88, 0x99));
      colorNames.put("lightslategrey", new RGB(0x77, 0x88, 0x99));
      colorNames.put("lightsteelblue", new RGB(0xb0, 0xc4, 0xde));
      colorNames.put("lightyellow", new RGB(0xff, 0xff, 0xe0));
      colorNames.put("lime", new RGB(0x00, 0xff, 0x00));
      colorNames.put("limegreen", new RGB(0x32, 0xcd, 0x32));
      colorNames.put("linen", new RGB(0xfa, 0xf0, 0xe6));
      colorNames.put("magenta", new RGB(0xff, 0x00, 0xff));
      colorNames.put("maroon", new RGB(0x80, 0x00, 0x00));
      colorNames.put("mediumaquamarine", new RGB(0x66, 0xcd, 0xaa));
      colorNames.put("mediumblue", new RGB(0x00, 0x00, 0xcd));
      colorNames.put("mediumorchid", new RGB(0xba, 0x55, 0xd3));
      colorNames.put("mediumpurple", new RGB(0x93, 0x70, 0xdb));
      colorNames.put("mediumseagreen", new RGB(0x3c, 0xb3, 0x71));
      colorNames.put("mediumslateblue", new RGB(0x7b, 0x68, 0xee));
      colorNames.put("mediumspringgreen", new RGB(0x00, 0xfa, 0x9a));
      colorNames.put("mediumturquoise", new RGB(0x48, 0xd1, 0xcc));
      colorNames.put("mediumvioletred", new RGB(0xc7, 0x15, 0x85));
      colorNames.put("midnightblue", new RGB(0x19, 0x19, 0x70));
      colorNames.put("mintcream", new RGB(0xf5, 0xff, 0xfa));
      colorNames.put("mistyrose", new RGB(0xff, 0xe4, 0xe1));
      colorNames.put("moccasin", new RGB(0xff, 0xe4, 0xb5));
      colorNames.put("navajowhite", new RGB(0xff, 0xde, 0xad));
      colorNames.put("navy", new RGB(0x00, 0x00, 0x80));
      colorNames.put("oldlace", new RGB(0xfd, 0xf5, 0xe6));
      colorNames.put("olive", new RGB(0x80, 0x80, 0x00));
      colorNames.put("olivedrab", new RGB(0x6b, 0x8e, 0x23));
      colorNames.put("orange", new RGB(0xff, 0xa5, 0x00));
      colorNames.put("orangered", new RGB(0xff, 0x45, 0x00));
      colorNames.put("orchid", new RGB(0xda, 0x70, 0xd6));
      colorNames.put("palegoldenrod", new RGB(0xee, 0xe8, 0xaa));
      colorNames.put("palegreen", new RGB(0x98, 0xfb, 0x98));
      colorNames.put("paleturquoise", new RGB(0xaf, 0xee, 0xee));
      colorNames.put("palevioletred", new RGB(0xdb, 0x70, 0x93));
      colorNames.put("papayawhip", new RGB(0xff, 0xef, 0xd5));
      colorNames.put("peachpuff", new RGB(0xff, 0xda, 0xb9));
      colorNames.put("peru", new RGB(0xcd, 0x85, 0x3f));
      colorNames.put("pink", new RGB(0xff, 0xc0, 0xcb));
      colorNames.put("plum", new RGB(0xdd, 0xa0, 0xdd));
      colorNames.put("powderblue", new RGB(0xb0, 0xe0, 0xe6));
      colorNames.put("purple", new RGB(0x80, 0x00, 0x80));
      colorNames.put("rebeccapurple", new RGB(0x66, 0x33, 0x99));
      colorNames.put("red", new RGB(0xff, 0x00, 0x00));
      colorNames.put("rosybrown", new RGB(0xbc, 0x8f, 0x8f));
      colorNames.put("royalblue", new RGB(0x41, 0x69, 0xe1));
      colorNames.put("saddlebrown", new RGB(0x8b, 0x45, 0x13));
      colorNames.put("salmon", new RGB(0xfa, 0x80, 0x72));
      colorNames.put("sandybrown", new RGB(0xf4, 0xa4, 0x60));
      colorNames.put("seagreen", new RGB(0x2e, 0x8b, 0x57));
      colorNames.put("seashell", new RGB(0xff, 0xf5, 0xee));
      colorNames.put("sienna", new RGB(0xa0, 0x52, 0x2d));
      colorNames.put("silver", new RGB(0xc0, 0xc0, 0xc0));
      colorNames.put("skyblue", new RGB(0x87, 0xce, 0xeb));
      colorNames.put("slateblue", new RGB(0x6a, 0x5a, 0xcd));
      colorNames.put("slategray", new RGB(0x70, 0x80, 0x90));
      colorNames.put("slategrey", new RGB(0x70, 0x80, 0x90));
      colorNames.put("snow", new RGB(0xff, 0xfa, 0xfa));
      colorNames.put("springgreen", new RGB(0x00, 0xff, 0x7f));
      colorNames.put("steelblue", new RGB(0x46, 0x82, 0xb4));
      colorNames.put("tan", new RGB(0xd2, 0xb4, 0x8c));
      colorNames.put("teal", new RGB(0x00, 0x80, 0x80));
      colorNames.put("thistle", new RGB(0xd8, 0xbf, 0xd8));
      colorNames.put("tomato", new RGB(0xff, 0x63, 0x47));
      colorNames.put("turquoise", new RGB(0x40, 0xe0, 0xd0));
      colorNames.put("violet", new RGB(0xee, 0x82, 0xee));
      colorNames.put("wheat", new RGB(0xf5, 0xde, 0xb3));
      colorNames.put("white", new RGB(0xff, 0xff, 0xff));
      colorNames.put("whitesmoke", new RGB(0xf5, 0xf5, 0xf5));
      colorNames.put("yellow", new RGB(0xff, 0xff, 0x00));
      colorNames.put("yellowgreen", new RGB(0x9a, 0xcd, 0x32));

      colorValues = new HashMap<RGB, String>();
      colorValues.put(new RGB(0xf0, 0xf8, 0xff), "AliceBlue");
      colorValues.put(new RGB(0xfa, 0xeb, 0xd7), "AntiqueWhite");
      colorValues.put(new RGB(0x00, 0xff, 0xff), "Aqua");
      colorValues.put(new RGB(0x7f, 0xff, 0xd4), "Aquamarine");
      colorValues.put(new RGB(0xf0, 0xff, 0xff), "Azure");
      colorValues.put(new RGB(0xf5, 0xf5, 0xdc), "Beige");
      colorValues.put(new RGB(0xff, 0xe4, 0xc4), "Bisque");
      colorValues.put(new RGB(0x00, 0x00, 0x00), "Black");
      colorValues.put(new RGB(0xff, 0xeb, 0xcd), "BlanchedAlmond");
      colorValues.put(new RGB(0x00, 0x00, 0xff), "Blue");
      colorValues.put(new RGB(0x8a, 0x2b, 0xe2), "BlueViolet");
      colorValues.put(new RGB(0xa5, 0x2a, 0x2a), "Brown");
      colorValues.put(new RGB(0xde, 0xb8, 0x87), "Burlywood");
      colorValues.put(new RGB(0x5f, 0x9e, 0xa0), "CadetBlue");
      colorValues.put(new RGB(0x7f, 0xff, 0x00), "Chartreuse");
      colorValues.put(new RGB(0xd2, 0x69, 0x1e), "Chocolate");
      colorValues.put(new RGB(0xff, 0x7f, 0x50), "Coral");
      colorValues.put(new RGB(0x64, 0x95, 0xed), "CornflowerBlue");
      colorValues.put(new RGB(0xff, 0xf8, 0xdc), "Cornsilk");
      colorValues.put(new RGB(0xdc, 0x14, 0x3c), "Crimson");
      colorValues.put(new RGB(0x00, 0xff, 0xff), "Cyan");
      colorValues.put(new RGB(0x00, 0x00, 0x8b), "DarkBlue");
      colorValues.put(new RGB(0x00, 0x8b, 0x8b), "DarkCyan");
      colorValues.put(new RGB(0xb8, 0x86, 0x0b), "DarkGoldenRod");
      colorValues.put(new RGB(0xa9, 0xa9, 0xa9), "DarkGray");
      colorValues.put(new RGB(0xa9, 0xa9, 0xa9), "DarkGrey");
      colorValues.put(new RGB(0x00, 0x64, 0x00), "DarkGreen");
      colorValues.put(new RGB(0xbd, 0xb7, 0x6b), "DarkKhaki");
      colorValues.put(new RGB(0x8b, 0x00, 0x8b), "DarkMagenta");
      colorValues.put(new RGB(0x55, 0x6b, 0x2f), "DarkoliveGreen");
      colorValues.put(new RGB(0xff, 0x8c, 0x00), "DarkOrange");
      colorValues.put(new RGB(0x99, 0x32, 0xcc), "Darkorchid");
      colorValues.put(new RGB(0x8b, 0x00, 0x00), "DarkRed");
      colorValues.put(new RGB(0xe9, 0x96, 0x7a), "DarkSalmon");
      colorValues.put(new RGB(0x8f, 0xbc, 0x8f), "DarkseaGreen");
      colorValues.put(new RGB(0x48, 0x3d, 0x8b), "DarkSlateBlue");
      colorValues.put(new RGB(0x2f, 0x4f, 0x4f), "DarkSlateGray");
      colorValues.put(new RGB(0x2f, 0x4f, 0x4f), "DarkSlateGrey");
      colorValues.put(new RGB(0x00, 0xce, 0xd1), "DarkTurquoise");
      colorValues.put(new RGB(0x94, 0x00, 0xd3), "DarkViolet");
      colorValues.put(new RGB(0xff, 0x14, 0x93), "DeepPink");
      colorValues.put(new RGB(0x00, 0xbf, 0xff), "DeepskyBlue");
      colorValues.put(new RGB(0x69, 0x69, 0x69), "DimGray");
      colorValues.put(new RGB(0x69, 0x69, 0x69), "DimGrey");
      colorValues.put(new RGB(0x1e, 0x90, 0xff), "DodgerBlue");
      colorValues.put(new RGB(0xb2, 0x22, 0x22), "Firebrick");
      colorValues.put(new RGB(0xff, 0xfa, 0xf0), "FloralWhite");
      colorValues.put(new RGB(0x22, 0x8b, 0x22), "ForestGreen");
      colorValues.put(new RGB(0xff, 0x00, 0xff), "Fuchsia");
      colorValues.put(new RGB(0xdc, 0xdc, 0xdc), "Gainsboro");
      colorValues.put(new RGB(0xf8, 0xf8, 0xff), "GhostWhite");
      colorValues.put(new RGB(0xff, 0xd7, 0x00), "Gold");
      colorValues.put(new RGB(0xda, 0xa5, 0x20), "GoldenRod");
      colorValues.put(new RGB(0x80, 0x80, 0x80), "Gray");
      colorValues.put(new RGB(0x80, 0x80, 0x80), "Grey");
      colorValues.put(new RGB(0x00, 0x80, 0x00), "Green");
      colorValues.put(new RGB(0xad, 0xff, 0x2f), "GreenYellow");
      colorValues.put(new RGB(0xf0, 0xff, 0xf0), "HoneyDew");
      colorValues.put(new RGB(0xff, 0x69, 0xb4), "HotPink");
      colorValues.put(new RGB(0xcd, 0x5c, 0x5c), "IndianRed");
      colorValues.put(new RGB(0x4b, 0x00, 0x82), "Indigo");
      colorValues.put(new RGB(0xff, 0xff, 0xf0), "Ivory");
      colorValues.put(new RGB(0xf0, 0xe6, 0x8c), "Khaki");
      colorValues.put(new RGB(0xe6, 0xe6, 0xfa), "Lavender");
      colorValues.put(new RGB(0xff, 0xf0, 0xf5), "LavenderBlush");
      colorValues.put(new RGB(0x7c, 0xfc, 0x00), "LawnGreen");
      colorValues.put(new RGB(0xff, 0xfa, 0xcd), "LemonChiffon");
      colorValues.put(new RGB(0xad, 0xd8, 0xe6), "LightBlue");
      colorValues.put(new RGB(0xf0, 0x80, 0x80), "LightCoral");
      colorValues.put(new RGB(0xe0, 0xff, 0xff), "LightCyan");
      colorValues.put(new RGB(0xfa, 0xfa, 0xd2), "LightGoldenRodYellow");
      colorValues.put(new RGB(0xd3, 0xd3, 0xd3), "LightGray");
      colorValues.put(new RGB(0xd3, 0xd3, 0xd3), "LightGrey");
      colorValues.put(new RGB(0x90, 0xee, 0x90), "LightGreen");
      colorValues.put(new RGB(0xff, 0xb6, 0xc1), "LightPink");
      colorValues.put(new RGB(0xff, 0xa0, 0x7a), "LightSalmon");
      colorValues.put(new RGB(0x20, 0xb2, 0xaa), "LightseaGreen");
      colorValues.put(new RGB(0x87, 0xce, 0xfa), "LightskyBlue");
      colorValues.put(new RGB(0x77, 0x88, 0x99), "LightSlateGray");
      colorValues.put(new RGB(0x77, 0x88, 0x99), "LightSlateGrey");
      colorValues.put(new RGB(0xb0, 0xc4, 0xde), "LightsteelBlue");
      colorValues.put(new RGB(0xff, 0xff, 0xe0), "LightYellow");
      colorValues.put(new RGB(0x00, 0xff, 0x00), "Lime");
      colorValues.put(new RGB(0x32, 0xcd, 0x32), "LimeGreen");
      colorValues.put(new RGB(0xfa, 0xf0, 0xe6), "Linen");
      colorValues.put(new RGB(0xff, 0x00, 0xff), "Magenta");
      colorValues.put(new RGB(0x80, 0x00, 0x00), "Maroon");
      colorValues.put(new RGB(0x66, 0xcd, 0xaa), "Mediumaquamarine");
      colorValues.put(new RGB(0x00, 0x00, 0xcd), "MediumBlue");
      colorValues.put(new RGB(0xba, 0x55, 0xd3), "Mediumorchid");
      colorValues.put(new RGB(0x93, 0x70, 0xdb), "MediumPurple");
      colorValues.put(new RGB(0x3c, 0xb3, 0x71), "MediumseaGreen");
      colorValues.put(new RGB(0x7b, 0x68, 0xee), "MediumSlateBlue");
      colorValues.put(new RGB(0x00, 0xfa, 0x9a), "MediumSpringGreen");
      colorValues.put(new RGB(0x48, 0xd1, 0xcc), "MediumTurquoise");
      colorValues.put(new RGB(0xc7, 0x15, 0x85), "MediumVioletRed");
      colorValues.put(new RGB(0x19, 0x19, 0x70), "MidnightBlue");
      colorValues.put(new RGB(0xf5, 0xff, 0xfa), "Mintcream");
      colorValues.put(new RGB(0xff, 0xe4, 0xe1), "Mistyrose");
      colorValues.put(new RGB(0xff, 0xe4, 0xb5), "Moccasin");
      colorValues.put(new RGB(0xff, 0xde, 0xad), "NavajoWhite");
      colorValues.put(new RGB(0x00, 0x00, 0x80), "Navy");
      colorValues.put(new RGB(0xfd, 0xf5, 0xe6), "Oldlace");
      colorValues.put(new RGB(0x80, 0x80, 0x00), "Olive");
      colorValues.put(new RGB(0x6b, 0x8e, 0x23), "Olivedrab");
      colorValues.put(new RGB(0xff, 0xa5, 0x00), "Orange");
      colorValues.put(new RGB(0xff, 0x45, 0x00), "OrangeRed");
      colorValues.put(new RGB(0xda, 0x70, 0xd6), "Orchid");
      colorValues.put(new RGB(0xee, 0xe8, 0xaa), "PaleGoldenRod");
      colorValues.put(new RGB(0x98, 0xfb, 0x98), "PaleGreen");
      colorValues.put(new RGB(0xaf, 0xee, 0xee), "PaleTurquoise");
      colorValues.put(new RGB(0xdb, 0x70, 0x93), "PaleVioletRed");
      colorValues.put(new RGB(0xff, 0xef, 0xd5), "Papayawhip");
      colorValues.put(new RGB(0xff, 0xda, 0xb9), "Peachpuff");
      colorValues.put(new RGB(0xcd, 0x85, 0x3f), "Peru");
      colorValues.put(new RGB(0xff, 0xc0, 0xcb), "Pink");
      colorValues.put(new RGB(0xdd, 0xa0, 0xdd), "Plum");
      colorValues.put(new RGB(0xb0, 0xe0, 0xe6), "PowderBlue");
      colorValues.put(new RGB(0x80, 0x00, 0x80), "Purple");
      colorValues.put(new RGB(0x66, 0x33, 0x99), "RebeccaPurple");
      colorValues.put(new RGB(0xff, 0x00, 0x00), "Red");
      colorValues.put(new RGB(0xbc, 0x8f, 0x8f), "RosyBrown");
      colorValues.put(new RGB(0x41, 0x69, 0xe1), "RoyalBlue");
      colorValues.put(new RGB(0x8b, 0x45, 0x13), "SaddleBrown");
      colorValues.put(new RGB(0xfa, 0x80, 0x72), "Salmon");
      colorValues.put(new RGB(0xf4, 0xa4, 0x60), "SandyBrown");
      colorValues.put(new RGB(0x2e, 0x8b, 0x57), "SeaGreen");
      colorValues.put(new RGB(0xff, 0xf5, 0xee), "SeaShell");
      colorValues.put(new RGB(0xa0, 0x52, 0x2d), "Sienna");
      colorValues.put(new RGB(0xc0, 0xc0, 0xc0), "Silver");
      colorValues.put(new RGB(0x87, 0xce, 0xeb), "SkyBlue");
      colorValues.put(new RGB(0x6a, 0x5a, 0xcd), "SlateBlue");
      colorValues.put(new RGB(0x70, 0x80, 0x90), "SlateGray");
      colorValues.put(new RGB(0x70, 0x80, 0x90), "SlateGrey");
      colorValues.put(new RGB(0xff, 0xfa, 0xfa), "Snow");
      colorValues.put(new RGB(0x00, 0xff, 0x7f), "SpringGreen");
      colorValues.put(new RGB(0x46, 0x82, 0xb4), "SteelBlue");
      colorValues.put(new RGB(0xd2, 0xb4, 0x8c), "Tan");
      colorValues.put(new RGB(0x00, 0x80, 0x80), "Teal");
      colorValues.put(new RGB(0xd8, 0xbf, 0xd8), "Thistle");
      colorValues.put(new RGB(0xff, 0x63, 0x47), "Tomato");
      colorValues.put(new RGB(0x40, 0xe0, 0xd0), "Turquoise");
      colorValues.put(new RGB(0xee, 0x82, 0xee), "Violet");
      colorValues.put(new RGB(0xf5, 0xde, 0xb3), "wheat");
      colorValues.put(new RGB(0xff, 0xff, 0xff), "White");
      colorValues.put(new RGB(0xf5, 0xf5, 0xf5), "Whitesmoke");
      colorValues.put(new RGB(0xff, 0xff, 0x00), "Yellow");
      colorValues.put(new RGB(0x9a, 0xcd, 0x32), "YellowGreen");
   }

	/**
	 * Create integer value from Red/Green/Blue
	 * 
	 * @param r red
	 * @param g green
	 * @param b blue
	 * @return
	 */
	public static int rgbToInt(RGB rgb)
	{
		return rgb.red | (rgb.green << 8) | (rgb.blue << 16);
	}
	
	/**
	 * Create RGB object from integer value
	 * 
	 * @param color color as integer value
	 * @return RGB object
	 */
	public static RGB rgbFromInt(int color)
	{
		return new RGB(color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF);
	}


	/**
	 * Create Color object from integer RGB representation
	 * @param rgb color's rgb representation
	 * @param cache color cache where new color should be placed
	 * @return color object
	 */
	public static Color colorFromInt(int rgb, ColorCache cache)
	{
		// All colors on server stored as BGR: red in less significant byte and blue in most significant byte
		return cache.create(rgb & 0xFF, (rgb >> 8) & 0xFF, (rgb >> 16) & 0xFF);
	}

	/**
	 * Create color object from preference string
	 *  
	 * @param store preference store
	 * @param name preference name
	 * @param cache color cache where new color should be placed
	 * @return Color object
	 */
	public static Color getColorFromPreferences(IPreferenceStore store, final String name, ColorCache cache)
	{
		return cache.create(PreferenceConverter.getColor(store, name));
	}

	/**
	 * Get internal integer representation of color from preference string
	 *  
	 * @param store preference store
	 * @param name preference name
	 * @return color in BGR format
	 */
	public static int getColorFromPreferencesAsInt(IPreferenceStore store, final String name)
	{
		return rgbToInt(PreferenceConverter.getColor(store, name));
	}
	
	/**
	 * @param start
	 * @param end
	 * @param amount
	 * @return
	 */
	private static float lerp(float start, float end, float amount)
	{
		float difference = end - start;
		float adjusted = difference * amount;
		return start + adjusted;
	}
	
	/**
	 * Adjust given color in the direction of another color by given amount.
	 * For example, to make color 50% lighter:
	 * adjustColor(color, new Color(color.getDevice(), 255, 255, 255), 0.5f);
	 * 
	 * @param color
	 * @param direction
	 * @param amount
	 * @param cache color cache where new color should be placed
	 * @return
	 */
	public static Color adjustColor(Color color, RGB direction, float amount, ColorCache cache)
	{
		float sr = color.getRed(), sg = color.getGreen(), sb = color.getBlue();
		float dr = direction.red, dg = direction.green, db = direction.blue;
		return cache.create((int)lerp(sr, dr, amount), (int)lerp(sg, dg, amount), (int)lerp(sb, db, amount));
	}
	
	/**
	 * Convert given RGB value into CSS format
	 * 
	 * @param rgb rgb value
	 * @return color definition in CSS format
	 */
	public static String rgbToCss(RGB rgb)
	{
      if (rgb == null)
         return null;
	   String name = colorValues.get(rgb);
	   if (name != null)
	      return name;
	   int v = rgbToInt(rgb); // value is in BGR format
	   return String.format("#%02x%02x%02x", v & 0xFF, (v >> 8) & 0xFF, v >> 16);
	}

	/**
	 * Parse CSS compatible color definition
	 * 
	 * @param cdef color definition
	 * @return RGB object or null if input cannot be parsed
	 */
	public static RGB parseColorDefinition(String cdef)
	{
	   if ((cdef == null) || cdef.isEmpty())
	      return null;

	   if (cdef.startsWith("0x"))
      {
         try
         {
            int v = Integer.parseInt(cdef.substring(2), 16) & 0x00FFFFFF;
            return new RGB(v >> 16, (v >> 8) & 0xFF, v & 0xFF);
         }
         catch(NumberFormatException e)
         {
            return null;
         }
      }

      if (cdef.charAt(0) == '#')
      {
         try
         {
            int v = Integer.parseInt(cdef.substring(1), 16) & 0x00FFFFFF;
            return new RGB(v >> 16, (v >> 8) & 0xFF, v & 0xFF);
         }
         catch(NumberFormatException e)
         {
            return null;
         }
      }

      if (cdef.startsWith("rgb("))
      {
         int i = cdef.indexOf(')');
         if (i == -1)
            return null;
         String[] parts = cdef.substring(4, i).split(",");
         if (parts.length != 3)
            return null;
         
         try
         {
            int r = Integer.parseInt(parts[0]);
            int g = Integer.parseInt(parts[1]);
            int b = Integer.parseInt(parts[2]);
            if ((r < 0) || (r > 255) || (g < 0) || (g > 255) || (b < 0) || (b > 255))
               return null;
            return new RGB(r, g, b);
         }
         catch(NumberFormatException e)
         {
            return null;
         }
      }
      
	   return colorNames.get(cdef.toLowerCase());
	}
	
   /**
    * Select text color based on background color.
    *
    * @param color background color
    * @param cache color cache
    * @return proposed text color
    */
	public static Color selectTextColorByBackgroundColor(Color color, ColorCache cache)
	{
	   float arr[] = new float[3];
	   arr[0] = color.getRed();
      arr[1] = color.getGreen();
      arr[2] = color.getBlue();
	   
      for(int i = 0; i < arr.length ; i++)
      {
         arr[i] = arr[i]/255;
         if (arr[i] <= 0.03928) 
            arr[i] = (float)(arr[i]/12.92); 
         else 
            arr[i] = (float)Math.pow(((arr[i]+0.055)/1.055), 2.4);
      }
      float l = (float)(0.2126 * arr[0] + 0.7152 * arr[1] + 0.0722 * arr[2]);
      
      return l > 0.179 ? cache.create(rgbFromInt(0xFF000000)) : cache.create(rgbFromInt(0xFFFFFFFF));
	}

   /**
    * Check if given color is dark (lighting below 50 in CIELAB color space)
    *
    * @param rgb color to test
    * @return true if color is darker than 50 in CIELAB color space
    */
   public static boolean isDarkColor(RGB rgb)
   {
      return rgbToLab(rgb).l < 50;
   }

   /**
    * Check if given color is dark (lighting below 50 in CIELAB color space)
    *
    * @param color color to test
    * @return true if color is darker than 50 in CIELAB color space
    */
   public static boolean isDarkColor(Color color)
   {
      return isDarkColor(color.getRGB());
   }

   /**
    * Convert color value from RGB color space to CIELAB color space
    *
    * @param rgb rgb value
    * @return LAB value
    */
   public static CIELAB rgbToLab(RGB rgb)
   {
      double r, g, b, X, Y, Z, xr, yr, zr;

      // D65/2°
      double Xr = 95.047;
      double Yr = 100.0;
      double Zr = 108.883;

      // --------- RGB to XYZ ---------//

      r = rgb.red / 255.0;
      g = rgb.green / 255.0;
      b = rgb.blue / 255.0;

      if (r > 0.04045)
         r = Math.pow((r + 0.055) / 1.055, 2.4);
      else
         r = r / 12.92;

      if (g > 0.04045)
         g = Math.pow((g + 0.055) / 1.055, 2.4);
      else
         g = g / 12.92;

      if (b > 0.04045)
         b = Math.pow((b + 0.055) / 1.055, 2.4);
      else
         b = b / 12.92;

      r *= 100;
      g *= 100;
      b *= 100;

      X = 0.4124 * r + 0.3576 * g + 0.1805 * b;
      Y = 0.2126 * r + 0.7152 * g + 0.0722 * b;
      Z = 0.0193 * r + 0.1192 * g + 0.9505 * b;

      // --------- XYZ to Lab --------- //

      xr = X / Xr;
      yr = Y / Yr;
      zr = Z / Zr;

      if (xr > 0.008856)
         xr = (float)Math.pow(xr, 1 / 3.);
      else
         xr = (float)((7.787 * xr) + 16 / 116.0);

      if (yr > 0.008856)
         yr = (float)Math.pow(yr, 1 / 3.);
      else
         yr = (float)((7.787 * yr) + 16 / 116.0);

      if (zr > 0.008856)
         zr = (float)Math.pow(zr, 1 / 3.);
      else
         zr = (float)((7.787 * zr) + 16 / 116.0);

      return new CIELAB((float)(500 * (xr - yr)), (float)(200 * (yr - zr)), (float)((116 * yr) - 16));
   }

   /**
    * Blends two primary color components based on the provided ratio.
    *
    * @param v1 first component
    * @param v2 second component
    * @param ratio percentage of the first component in the blend
    * @return the blended color component
    */
   private static int blend(int v1, int v2, int ratio)
   {
      int b = (ratio * v1 + (100 - ratio) * v2) / 100;
      return Math.min(255, b);
   }

   /**
    * Blends c1 and c2 based in the provided ratio.
    *
    * @param c1 first color
    * @param c2 second color
    * @param ratio percentage of the first color in the blend (0-100)
    * @return the RGB value of the blended color
    * @since 3.1
    */
   public static RGB blend(RGB c1, RGB c2, int ratio)
   {
      int r = blend(c1.red, c2.red, ratio);
      int g = blend(c1.green, c2.green, ratio);
      int b = blend(c1.blue, c2.blue, ratio);
      return new RGB(r, g, b);
   }

   /**
    * Invert image colors.
    *
    * @param src source image
    * @return inverted image
    */
   public static ImageData invertImageColors(ImageData src)
   {
      ImageData dst = new ImageData(src.width, src.height, src.depth, src.palette);
      for(int y = 0; y < src.height; y++)
      {
         for(int x = 0; x < src.width; x++)
         {
            int alpha = src.getAlpha(x, y);
            if (alpha > 0)
            {
               RGB rgb = rgbFromInt(src.getPixel(x, y));
               rgb.blue = 255 - rgb.blue;
               rgb.green = 255 - rgb.green;
               rgb.red = 255 - rgb.red;
               dst.setPixel(x, y, rgbToInt(rgb));
            }
            else
            {
               dst.setPixel(x, y, src.getPixel(x, y));
            }
            dst.setAlpha(x, y, alpha);
         }
      }
      return dst;
   }
}
