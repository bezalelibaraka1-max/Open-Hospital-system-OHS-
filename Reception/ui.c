#include<stdio.h>
#include<stdlib.h>
#include<gtk/gtk.h>
#include<glib.h>//for g_malloc() and g_free()
#include<dirent.h>
#include "connection.h"


enum {
    COL_NAME,
    COL_AGE,
    COL_GENDER,
    NUM_COLS
};

    GtkBuilder *builder;
    GtkWidget *window;
    GtkTreeView *treeview;
    GtkListStore *store;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkWidget *dialog;
    GtkWidget *upload_dialog;

    void on_save_clicked(GtkButton *button, gpointer data)
    {

        treeview = GTK_TREE_VIEW(data);

        GtkWidget *dialog;
        dialog = gtk_file_chooser_dialog_new(
            "save file",GTK_WINDOW(window),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "cancel",GTK_RESPONSE_CANCEL,
            "save",GTK_RESPONSE_ACCEPT,
            NULL);


             /* Suggest a default filename */
            gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog),"data.txt");

            if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT)
            {
                char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

                  /* Get the model behind the TreeView */
                model =gtk_tree_view_get_model(treeview);

                 FILE *file = fopen(filename, "w");

                if (file == NULL)
                {g_printerr("Could not open file for writing\n");
                g_free(filename);
                gtk_widget_destroy(dialog);
                return;}

                GtkTreeIter iter;

                gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
                   /* Start from the first row */

                  while (valid)
                  {
                   gchar *name;
                   gint age;
                   gchar *gender;

                   gtk_tree_model_get(model,&iter,
                    COL_NAME, &name,
                    COL_AGE, &age,
                    COL_GENDER, &gender,
                    -1 );

                   fprintf(file,"%s\t%d\t%s\n",name,age,gender);
                      g_free(name);
                      g_free(gender);

                    /* Move to next row */
                   valid = gtk_tree_model_iter_next(model,&iter);
                }

                fclose(file);

                g_free(filename);
    }

    gtk_widget_destroy(dialog);
}


typedef struct {
    GtkEntry *name_entry;
    GtkEntry *age_entry;
    GtkComboBoxText *gender_combo;
} AppData;


void on_add_clicked(GtkButton *button, gpointer user_data)
{

    AppData *data = (AppData *)user_data;

    const char *name =
        gtk_entry_get_text(data->name_entry);

    const char *age =
        gtk_entry_get_text(data->age_entry);

        int ageno = atoi(age);

    gchar *gender = gtk_combo_box_text_get_active_text(data->gender_combo);


    // Don't add an empty name and age
    if (name == NULL || *name == '\0')
    {return;}

    if (!gender)
    {return;}

    if(ageno == 0)
    {return;}
    
    // Add a new row to the ListStore
    gtk_list_store_append(store, &iter);

    // Put the name into column 0 and age in column 1
    gtk_list_store_set(store, &iter,0, name,1,ageno,2,gender,-1);

    g_free(gender);

    // Clear the entry after adding
     gtk_entry_set_text(data->name_entry, "");
     gtk_entry_set_text(data->age_entry, "");

}

typedef struct{
    GtkTextBuffer *txtbuf;
}txt;

void on_send_dialog_clicked(GtkButton *button, gpointer userdata)
{
    txt *data = (txt*)userdata;
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(data->txtbuf,&start,&end);
    char*res=gtk_text_buffer_get_text(data->txtbuf,&start,&end,FALSE);

    send_file(res);
    g_free(res);
    gtk_widget_destroy(upload_dialog);
}

void on_cancel_dialog_clicked(GtkButton *button, gpointer data)
{
    
    gtk_widget_destroy(upload_dialog);
}

void on_upload_clicked(GtkButton *button, gpointer user_data)
{
    txt *data = (txt*)user_data;

   char *buffer = NULL;
dialog = gtk_file_chooser_dialog_new("open file",GTK_WINDOW(window),
GTK_FILE_CHOOSER_ACTION_OPEN,"cancel",GTK_RESPONSE_CANCEL,
"open",GTK_RESPONSE_ACCEPT,NULL);

if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT)
{
    char *name_of_file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    FILE*fp = fopen(name_of_file,"rb");
    g_free(name_of_file);
    if(fp){
    gtk_list_store_clear(store);
        fseek(fp,0,SEEK_END);
        long size = ftell(fp);
        rewind(fp); 

        buffer = malloc(size+1);

        size_t n = fread(buffer,1,size,fp);
        buffer[n] = '\0';
        
    }
    fclose(fp);

    gtk_widget_destroy(dialog);

    if(!buffer) return;

    builder = gtk_builder_new_from_file("upload_dialog.glade");
    upload_dialog = GTK_WIDGET(gtk_builder_get_object(builder,"upload_dialog"));
    gtk_window_set_default_size(GTK_WINDOW(upload_dialog),400,300);
    GtkWidget* textview = GTK_WIDGET(gtk_builder_get_object(builder,"textsee"));
    data->txtbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
    gtk_text_buffer_set_text(data->txtbuf,buffer,-1);



     GtkWidget *kancel= GTK_WIDGET(gtk_builder_get_object(builder,"kancel"));
     g_signal_connect(kancel, "clicked",G_CALLBACK(on_cancel_dialog_clicked),NULL);


     GtkWidget *send_dialog= GTK_WIDGET(gtk_builder_get_object(builder,"okay"));
     g_signal_connect(send_dialog, "clicked",G_CALLBACK(on_send_dialog_clicked),data);
    
    gtk_widget_show_all(upload_dialog);
    gtk_dialog_run(GTK_DIALOG(upload_dialog));

    free(buffer);
    g_free(name_of_file);

} else{
    gtk_widget_destroy(dialog);
}
}



int main(int argc, char*argv[])
{

    gtk_init(&argc, &argv);
    builder=gtk_builder_new_from_file("reception.glade");
    window=GTK_WIDGET(gtk_builder_get_object(builder,"mainwindow"));
    gtk_window_set_default_size(GTK_WINDOW(window),700,500);


    AppData *data = g_malloc(sizeof(AppData));

    data->name_entry = GTK_ENTRY(gtk_builder_get_object(builder, "name"));

    data->age_entry = GTK_ENTRY(gtk_builder_get_object(builder, "age"));

    data->gender_combo = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "gender"));



    treeview = GTK_TREE_VIEW(gtk_builder_get_object(builder, "treeview"));
    gtk_widget_set_name(GTK_WIDGET(treeview), "editor");

    store = gtk_list_store_new(3,G_TYPE_STRING,G_TYPE_INT,G_TYPE_STRING);

    gtk_tree_view_set_model(
        GTK_TREE_VIEW(treeview),
        GTK_TREE_MODEL(store));

    
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Name",renderer,"text",0,NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview),column);

    renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *age_column = gtk_tree_view_column_new_with_attributes("Age",renderer,"text",1,NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview),age_column);

    renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *gender_column = gtk_tree_view_column_new_with_attributes("Gender",renderer,"text",2,NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview),gender_column);


     GtkWidget *save = GTK_WIDGET(gtk_builder_get_object(builder,"save"));
     g_signal_connect(save, "clicked",G_CALLBACK(on_save_clicked),treeview);
     gtk_widget_set_name(save, "savebtn");

     GtkWidget *add = GTK_WIDGET(gtk_builder_get_object(builder,"add"));
     g_signal_connect(add, "clicked",G_CALLBACK(on_add_clicked),data);
     gtk_widget_set_name(add, "addbtn");

     GtkWidget *upload = GTK_WIDGET(gtk_builder_get_object(builder,"upload"));
     g_signal_connect(upload,"clicked",G_CALLBACK(on_upload_clicked),data);
     gtk_widget_set_name(upload,"uploadbtn");

    GtkCssProvider*provider=gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider,"reception.css",NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
