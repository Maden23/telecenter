#include "singleStreamUI.h"

SingleStreamUI::SingleStreamUI()
{
    gtk_init (nullptr, nullptr);

    /* Init windows */
    playerBuilder = nullptr;
    playerWindow = windowInit(&playerBuilder, "ui/player.glade", "player");

    if (!playerBuilder || !playerBuilder)
    {
        cerr << "Failed to init window" << endl << endl;
        exit(1);
    }
    
    /* Set window size */
    gtk_widget_set_size_request(playerWindow, 1080, 720);
	
    /* Init styles */
    int res = initStyles();
    if(res == -1) {
        cerr << "Failed to get styles" << endl << endl;
    }

    /* Find player and window elements to control*/
    initPlayerWidgets();
    player = new Player(playerWidget, "other");
    player->setCam("520 cam", "rtsp://172.18.212.17:554/Streaming/Channels/101?transportmode=unicast&profile=Profile_1");
    player->playStream();

    g_signal_connect(playerWindow, "destroy", G_CALLBACK (gtk_main_quit), NULL);
    gtk_window_present(GTK_WINDOW(playerWindow));
    
    gtk_main();

}

SingleStreamUI::~SingleStreamUI()
{
    delete player;
}

int SingleStreamUI::initStyles() {
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

GtkWidget* SingleStreamUI::windowInit(GtkBuilder** builder, string gladeFile, string windowName) {
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

void SingleStreamUI::initPlayerWidgets()
{
    /* Find drawing area*/
    playerWidget = (GtkWidget*)gtk_builder_get_object(playerBuilder, "playerWidget");
    if (!playerWidget)
    {
        cerr << "Player widget not found." << endl;
    }

    /* Find cam label */
    playerLabel = (GtkWidget*)gtk_builder_get_object(playerBuilder, "playerLabel");
    if (!playerLabel)
    {
        cerr << "Player label not found." << endl;
    }
}