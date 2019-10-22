#include "ui.h"

UI::UI(Config *config)
{
    this->config = config;

    gtk_init (nullptr, nullptr);

    /* Init styles */
    int res = initStyles();

    if(res == -1) {
        cerr << "Failed to get styles" << endl;
    }

    /* Init windows */
    menuBuilder = nullptr;
    menuWindow = windowInit(&menuBuilder, "ui/menu_player.glade", "menu");
    
    if(menuWindow == nullptr || menuBuilder == nullptr) {
        cerr << "Failed to init builder" << endl;
        exit(0);
    }

    /* Set window size */
    gtk_widget_set_size_request(menuWindow, stoi(config->getParam("windowWidth")),
            stoi(config->getParam("windowHight")));



    /* Find elements to control*/
    initPlayerWidgets();
    player = new Player(playerWidget, playerLabel, config);
    initMenuWidgets();


    g_signal_connect(menuWindow, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    gtk_window_present(GTK_WINDOW(menuWindow));
    // gtk_widget_show(menuWindow);
    gtk_main();
}

UI::~UI()
{
    delete player;
}


int UI::initStyles() {
    GFile* css = g_file_new_for_path("ui/styles.css");
    if(css == nullptr) {
      std::cout << "File not found" << std::endl;
      return -1;
    }
    GError* err = nullptr;
    GtkCssProvider *css_provider =  gtk_css_provider_new();
    gtk_css_provider_load_from_file(css_provider, css, &err);
    if(err != nullptr) {
      cerr << "Error occured while loading styles: " <<  err->message;
      g_error_free(err);
      return -1;
    }
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), (GtkStyleProvider*) css_provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
    return 0;
}

GtkWidget* UI::windowInit(GtkBuilder** builder, string gladeFile, string windowName) {
    ifstream f(gladeFile.c_str());
    if (!f.good())
    {
        cerr << "Could not find " << gladeFile << endl;
        return nullptr;
    }

    *builder = gtk_builder_new();

    if (builder == nullptr) {
        cerr << "No builder for " << gladeFile << endl;
        return nullptr;
    }

    if(gtk_builder_add_from_file(*builder, gladeFile.c_str(), nullptr) == 0) {
        printf("Build failed\n");
        return nullptr;
    }

    GtkWidget* window = GTK_WIDGET (gtk_builder_get_object(*builder, windowName.c_str()));

    if(window == nullptr) {
        cerr << "No window for " << gladeFile << endl;
        return nullptr;
    }

    gtk_widget_set_size_request (GTK_WIDGET(window), 1920, 550);


    gtk_builder_connect_signals(*builder,nullptr);

    g_signal_connect (G_OBJECT (window), "destroy", gtk_main_quit, nullptr);

    return window;
}


void UI::initPlayerWidgets()
{
    /* Find drawing area*/
    playerWidget = (GtkWidget*)gtk_builder_get_object(menuBuilder, "playerWidget");
    if (!playerWidget)
    {
        cerr << "Player widget not found." << endl;
    }



    /* Find cam label */
    playerLabel = (GtkWidget*)gtk_builder_get_object(menuBuilder, "playerLabel");
    if (!playerLabel)
    {
        cerr << "Player label not found." << endl;
    }
    /* If clicked on cam button */
    struct hide_player_data* data = new struct hide_player_data;
    data->playerWidget = playerWidget;
    data->playerLabel = playerLabel;
    g_signal_connect(G_OBJECT(playerLabel), "clicked", G_CALLBACK(hidePlayer), data);
}


void UI::initMenuWidgets()
{
    /* Find out how many cameras are known */
    int camCount = config->getCamCount();
    if (camCount > 9)
    {
        cout << "Too many cameras to display" << endl;
    }

    map<string, string> cams = config->getCams();

    /*  Setting buttons for each camera. Remember not to run out of 9 buttons */
    int n = 0;
    for (auto &cam : cams)
    {   
        if (n == 9) break;

        /* Find the button object */
        string name = "b" + to_string(n);
        GtkWidget *button = (GtkWidget*) gtk_builder_get_object(menuBuilder, name.c_str());
        
        if (button == nullptr)
        {
            cerr << "Cannot get button " << n << endl;
            continue;
        }
        /* Show button and assign cam label */
        gtk_widget_show(button);
        gtk_button_set_label(GTK_BUTTON(button), cam.first.c_str());

        /* Pass player and cam_id to callback function */
        struct display_player_data *data = new struct display_player_data;
        data->playerWidget = playerWidget;
        data->playerLabel = playerLabel;
        data->player = player;
        data->cam_id = cam.first;

        g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(displayPlayer), data);

        /* Find the rec image object */
        name = "i" + to_string(n);
        GtkWidget *recImage = (GtkWidget*) gtk_builder_get_object(menuBuilder, name.c_str());
        if (recImage == nullptr)
        {
            cerr << "Cannot get rec image " << n << endl;
            continue;
        }

        /* Assign cam ids to elements */
        buttons[cam.first] = button;
        recImages[cam.first] = recImage;

        n++;

    }
}


void displayPlayer(GtkWidget* widget, gpointer *data)
{
    auto context = (struct display_player_data*) data;
    // gtk_widget_hide(GTK_WIDGET(context->menuWindow));
    gtk_widget_show(context->playerWidget);
    gtk_widget_show(context->playerLabel);
    context->player->playStream(context->cam_id);
    // delete data;

}


void hidePlayer(GtkWidget* widget, gpointer *data)
{
    auto context = (struct hide_player_data*) data;
    gtk_widget_hide(context->playerWidget);
    gtk_widget_hide(context->playerLabel);

}