#include<stdio.h>
#include<stdlib.h>
#include<gtk/gtk.h>
#include "ftp.h"

GtkWidget * window;

void on_button_clicked(GtkButton *button,gpointer data)
{
GtkWidget *dialog=gtk_file_chooser_dialog_new("select file",NULL,
GTK_FILE_CHOOSER_ACTION_SAVE,
"cancel",GTK_RESPONSE_CANCEL,
"save",GTK_RESPONSE_ACCEPT,
NULL);

if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT)
{
    char *filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    send_file(filename);
    g_free(filename);
    gtk_widget_destroy(dialog);
}
}
int main(int argc,char*argv[])
{
    GtkBuilder*builder;
    GtkWidget *window;
    gtk_init(&argc,&argv);
    
    builder=gtk_builder_new_from_file("send.glade");
    window=GTK_WIDGET(gtk_builder_get_object(builder,"main"));
    gtk_window_set_default_size(GTK_WINDOW(window),1280,720);


    GtkWidget *btn = GTK_WIDGET(gtk_builder_get_object(builder,"send"));
    g_signal_connect(btn, "clicked",G_CALLBACK(on_button_clicked),NULL);

    GtkCssProvider*provider=gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider,"send.css",NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}