#include "ui.h"

UI::UI(Config *config)
{
    this->config = config;

    gtk_init (nullptr, nullptr);

    /* Init styles */
    // int res = initStyles();

    // if(res == -1) {
    //     cerr << "Cannot get styles" << endl;
    //     return 0;
    // }

    /* Init windows */
    menuBuilder = nullptr;
    menuWindow = windowInit(&menuBuilder, "menu.glade");
    playerBuilder = nullptr;
    playerWindow = windowInit(&playerBuilder, "player.glade");

    if(menuWindow == nullptr || menuBuilder == nullptr
    || playerWindow == nullptr || playerBuilder == nullptr) {
        cerr << "Cannot init builder" << endl;
        exit(0);
    }

    /* Set window size */
    gtk_widget_set_size_request(menuWindow, stoi(config->getParam("windowWidth")),
            stoi(config->getParam("windowHight")));
    gtk_widget_set_size_request(playerWindow, stoi(config->getParam("windowWidth")),
            stoi(config->getParam("windowHight")));

    /* Find elements to control*/
    initPlayerWidgets();
    initMenuWidgets();

    gtk_widget_show(menuWindow);
    gtk_main();
}

UI::~UI()
{

}


// static int initStyles() {
//     GFile* css = g_file_new_for_path("styles.css");
//     if(css == nullptr) {
//       std::cout << "File not found" << std::endl;
//       return -1;
//     }
//     GError* err = nullptr;
//     GtkCssProvider *css_provider =  gtk_css_provider_new();
//     gtk_css_provider_load_from_file(css_provider, css, &err);
//     if(err != nullptr) {
//       g_printerr("Err: ", err->message);
//       g_error_free(err);
//       return -1;
//     }
//     gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), (GtkStyleProvider*) css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
//     return 0;
// }

GtkWidget* UI::windowInit(GtkBuilder** builder, string gladeFile) {
    *builder = gtk_builder_new();

    if (builder == nullptr) {
        cout << "No builder" << endl;
        return nullptr;
    }

    if(gtk_builder_add_from_file(*builder, gladeFile.c_str(), nullptr) == 0) {
        printf("build failed\n");
        return nullptr;
    }

    GtkWidget* window = GTK_WIDGET (gtk_builder_get_object(*builder, 
        gladeFile.substr(0, gladeFile.find(".")).c_str()));

    if(window == nullptr) {
        cout << "No window" << endl;
        return nullptr;
    }

    gtk_widget_set_size_request (GTK_WIDGET(window), 1920, 550);


    gtk_builder_connect_signals(*builder,nullptr);

    g_signal_connect (G_OBJECT (window), "destroy", gtk_main_quit, nullptr);

    return window;
}


void UI::initPlayerWidgets()
{
    playerWidget = (GtkWidget*)gtk_builder_get_object(playerBuilder, "playerWidget");
    if (!playerWidget)
    {
        cerr << "Player widget not found." << endl;
    }

    playerLabel = (GtkWidget*)gtk_builder_get_object(playerBuilder, "playerLabel");
    if (!playerLabel)
    {
        cerr << "Player label not found." << endl;
    }
}


void UI::initMenuWidgets()
{
    // GtkWidget* grid = gtk_bin_get_child((GtkBin*)menuWindow);
    // GList* overlays = gtk_container_get_children((GtkContainer*)grid);
    // GList* o;
    // for (o = overlays; o != NULL; o = o->next)
    // {
    //     GList* menuWidgets = gtk_container_get_children((GtkContainer*)o->data);
    //     GList* w;
    //     for (w = menuWidgets; w != NULL; w = w->next);
    //     {
    //         if (w->data) cout << "widget" << endl;
    //         GtkWidget *widget = (GtkWidget*)w->data;
    //         if (GTK_IS_BUTTON(widget))
    //         {
    //             cout << gtk_widget_get_name(widget) << endl;
    //         }
    //         if (GTK_IS_IMAGE(widget))
    //         {
    //             cout << gtk_widget_get_name(widget) << endl;
    //         }
    //     }
    // }

    /* Find out how many cameras are used */

    for (int i = 0; i < 9; i++)
    {
        /* Find button object */
        string name = "b" + to_string(i);
        GtkWidget *button = (GtkWidget*) gtk_builder_get_object(menuBuilder, name.c_str());
        if (button == nullptr)
        {
            cerr << "Cannot get name";
        }
    }
}
