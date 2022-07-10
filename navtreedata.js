/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "NimBLE-Arduino", "index.html", [
    [ "Overview", "index.html", [
      [ "What is NimBLE?", "index.html#autotoc_md11", null ],
      [ "Arduino installation", "index.html#autotoc_md12", null ],
      [ "Platformio installation", "index.html#autotoc_md13", null ],
      [ "Using", "index.html#autotoc_md14", [
        [ "Examples", "index.html#autotoc_md15", null ],
        [ "Arduino command line and platformio", "index.html#autotoc_md16", null ]
      ] ],
      [ "Need help? Have a question or suggestion?", "index.html#autotoc_md17", null ],
      [ "Acknowledgments", "index.html#autotoc_md18", null ]
    ] ],
    [ "Bluetooth 5.x features", "md__bluetooth_5_features.html", [
      [ "About extended advertising", "md__bluetooth_5_features.html#autotoc_md1", null ],
      [ "Enabling extended advertising", "md__bluetooth_5_features.html#autotoc_md2", null ]
    ] ],
    [ "Arduino command line and platformio config options", "md__command_line_config.html", null ],
    [ "Improvements and updates", "md__improvements_and_updates.html", [
      [ "Server", "md__improvements_and_updates.html#autotoc_md6", null ],
      [ "Advertising", "md__improvements_and_updates.html#autotoc_md7", null ],
      [ "Client", "md__improvements_and_updates.html#autotoc_md8", null ],
      [ "General", "md__improvements_and_updates.html#autotoc_md9", null ]
    ] ],
    [ "Migrating from Bluedroid to NimBLE", "md__migration_guide.html", [
      [ "General Information", "md__migration_guide.html#autotoc_md20", [
        [ "Extended advertising settings, For use with ESP32C3, ESP32S3, ESP32H2 ONLY!", "md__command_line_config.html#autotoc_md4", null ],
        [ "Header Files", "md__migration_guide.html#autotoc_md21", null ],
        [ "Class Names", "md__migration_guide.html#autotoc_md22", null ],
        [ "BLE Addresses", "md__migration_guide.html#autotoc_md23", null ]
      ] ],
      [ "Server API", "md__migration_guide.html#autotoc_md24", [
        [ "Services", "md__migration_guide.html#autotoc_md25", null ],
        [ "Characteristics", "md__migration_guide.html#autotoc_md26", [
          [ "Originally", "md__migration_guide.html#autotoc_md27", null ],
          [ "Is Now", "md__migration_guide.html#autotoc_md28", null ],
          [ "The full list of properties", "md__migration_guide.html#autotoc_md29", null ]
        ] ],
        [ "Descriptors", "md__migration_guide.html#autotoc_md30", null ],
        [ "Server Security", "md__migration_guide.html#autotoc_md33", null ]
      ] ],
      [ "Advertising API", "md__migration_guide.html#autotoc_md34", null ],
      [ "Client API", "md__migration_guide.html#autotoc_md35", [
        [ "Remote Services", "md__migration_guide.html#autotoc_md36", null ],
        [ "Remote Characteristics", "md__migration_guide.html#autotoc_md37", null ],
        [ "Client Security", "md__migration_guide.html#autotoc_md38", null ]
      ] ],
      [ "Security API", "md__migration_guide.html#autotoc_md39", null ],
      [ "Arduino Configuration", "md__migration_guide.html#autotoc_md40", null ]
    ] ],
    [ "New User Guide", "md__new_user_guide.html", [
      [ "Include Files", "md__new_user_guide.html#autotoc_md42", null ],
      [ "Using the Library", "md__new_user_guide.html#autotoc_md43", null ],
      [ "Creating a Server", "md__new_user_guide.html#autotoc_md44", null ],
      [ "Creating a Client", "md__new_user_guide.html#autotoc_md45", null ]
    ] ],
    [ "Usage Tips", "md__usage_tips.html", [
      [ "Put BLE functions in a task running on the NimBLE stack core", "md__usage_tips.html#autotoc_md47", null ],
      [ "Do not delete client instances unless necessary or unused", "md__usage_tips.html#autotoc_md48", null ],
      [ "Only retrieve the services and characteristics needed", "md__usage_tips.html#autotoc_md49", null ],
      [ "Check return values", "md__usage_tips.html#autotoc_md50", null ],
      [ "There will be bugs - please report them", "md__usage_tips.html#autotoc_md51", null ]
    ] ],
    [ "Changelog", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html", [
      [ "[1.4.0] - 2022-07-10", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md53", [
        [ "Fixed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md54", null ],
        [ "Changed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md55", null ],
        [ "Added", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md56", null ]
      ] ],
      [ "[1.3.8] - 2022-04-27", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md57", [
        [ "Fixed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md58", null ],
        [ "Changed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md59", null ]
      ] ],
      [ "[1.3.7] - 2022-02-15", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md60", [
        [ "Fixed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md61", null ]
      ] ],
      [ "[1.3.6] - 2022-01-18", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md62", [
        [ "Changed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md63", null ],
        [ "Fixed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md64", null ]
      ] ],
      [ "[1.3.5] - 2022-01-14", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md65", [
        [ "Added", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md66", null ],
        [ "Fixed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md67", null ]
      ] ],
      [ "[1.3.4] - 2022-01-09", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md68", [
        [ "Fixed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md69", null ]
      ] ],
      [ "[1.3.3] - 2021-11-24", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md70", [
        [ "Fixed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md71", null ]
      ] ],
      [ "[1.3.2] - 2021-11-20", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md72", [
        [ "Fixed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md73", null ],
        [ "Added", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md74", null ]
      ] ],
      [ "[1.3.1] - 2021-08-04", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md75", [
        [ "Fixed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md76", null ]
      ] ],
      [ "[1.3.0] - 2021-08-02", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md77", [
        [ "Added", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md78", null ],
        [ "Changed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md79", null ],
        [ "Fixed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md80", null ]
      ] ],
      [ "[1.2.0] - 2021-02-08", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md81", [
        [ "Added", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md82", null ],
        [ "Changed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md83", null ],
        [ "Fixed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md84", null ]
      ] ],
      [ "[1.1.0] - 2021-01-20", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md85", [
        [ "Added", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md86", null ],
        [ "Changed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md87", null ],
        [ "Fixed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md88", null ]
      ] ],
      [ "[1.0.2] - 2020-09-13", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md89", [
        [ "Changed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md90", null ]
      ] ],
      [ "[1.0.1] - 2020-09-02", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md91", [
        [ "Added", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md92", null ],
        [ "Changed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md93", null ],
        [ "Fixed", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md94", null ]
      ] ],
      [ "[1.0.0] - 2020-08-22", "md__k___users__ryan__documents__platform_i_o__projects__m_e_s_h_lib__nim_b_l_e__arduino__c_h_a_n_g_e_l_o_g.html#autotoc_md95", null ]
    ] ],
    [ "Deprecated List", "deprecated.html", null ],
    [ "Todo List", "todo.html", null ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Enumerations", "functions_enum.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"_h_i_d_keyboard_types_8h_source.html",
"class_nim_b_l_e_client.html#a668d476de250055a106a9f46bb7719f3",
"class_nim_b_l_e_security.html#ab2be50284a325ec8937abdab0baafd4b"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';