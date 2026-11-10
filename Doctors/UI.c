#include<gtk/gtk.h>
#include<stdio.h>
#include<dirent.h>
#include<string.h>
#include<limits.h>
#include<unistd.h>
#include<stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#define BUFFER_SIZE 8192
#define MAX_FILENAME 4096
#define BACKLOG 10

#include "communi.h"

#define PATH_SIZE 4096

GtkWidget *treeview;
GtkWidget *textview;
GtkListStore *store;

char current_dir[PATH_SIZE];

typedef struct {
    int port;
} AppData;




void load_directory(const char *path)
{

    DIR *dir;
    struct dirent *entry;

    gtk_list_store_clear(store);

    dir = opendir(path);

    if (dir == NULL)
    {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        /* Ignore . and .. */
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        GtkTreeIter iter;

        gtk_list_store_append(store, &iter);

        gtk_list_store_set(
            store,
            &iter,
            0,
            entry->d_name,
            -1
        );
    }

    closedir(dir);

    strncpy(current_dir, path, PATH_SIZE - 1);
    current_dir[PATH_SIZE - 1] = '\0';
}

void display_file(const char *filepath)
{
    FILE *file;
    char *buffer;
    long size;

    file = fopen(filepath, "r");

    if (file == NULL)
    {
        GtkTextBuffer *buffer_text =
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

        gtk_text_buffer_set_text(
            buffer_text,
            "Could not open file.",
            -1
        );

        return;
    }

    /* Find file size */
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);

    buffer = malloc(size + 1);

    if (buffer == NULL)
    {
        fclose(file);
        return;
    }

    fread(buffer, 1, size, file);

    buffer[size] = '\0';

    fclose(file);


/*For more safer
fseek(file, 0, SEEK_END);
long size = ftell(file);
rewind(file);

buffer = malloc(size + 1);

if (buffer == NULL)
{
    fclose(file);
    return;
}

size_t bytes_read = fread(buffer, 1, size, file);

buffer[bytes_read] = '\0';
fclose(file); */

    GtkTextBuffer *text_buffer =
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

    gtk_text_buffer_set_text(
        text_buffer,
        buffer,
        -1
    );

    free(buffer);
}

void on_file_activated(
    GtkTreeView *tree,
    GtkTreePath *path,
    GtkTreeViewColumn *column,
    gpointer user_data)
{
    GtkTreeModel *model;
    GtkTreeIter iter;

    model = gtk_tree_view_get_model(tree);

    if (!gtk_tree_model_get_iter(model, &iter, path))
        return;

    char *name;

    gtk_tree_model_get(
        model,
        &iter,
        0,
        &name,
        -1
    );

    char full_path[PATH_SIZE];

    snprintf(
        full_path,
        sizeof(full_path),
        "%s/%s",
        current_dir,
        name
    );

    struct stat info;

    if (stat(full_path, &info) == -1)
    {
        free(name);
        return;
    }

    /* Directory */
    if (S_ISDIR(info.st_mode))
    {
        load_directory(full_path);
    }

    /* Regular file */
    else if (S_ISREG(info.st_mode))
    {
        display_file(full_path);
    }

    free(name);
}


void go_back(GtkButton *button, gpointer user_data)
{
    char parent[PATH_SIZE];

    strncpy(parent, current_dir, PATH_SIZE - 1);
    parent[PATH_SIZE - 1] = '\0';

    char *slash = strrchr(parent, '/');

    if (slash != NULL && slash != parent)
    {
        *slash = '\0';
        load_directory(parent);
    }
}


void on_pharmacy_clicked(GtkButton *button,gpointer data)
{
    AppData *app = data;

GtkWidget *dialog=gtk_file_chooser_dialog_new("Send to pharmacy",NULL,
GTK_FILE_CHOOSER_ACTION_SAVE,
"cancel",GTK_RESPONSE_CANCEL,
"send",GTK_RESPONSE_ACCEPT,
NULL);

if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT)
{
    char *filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    communicate(app->port,filename);

    g_free(filename);

    gtk_widget_destroy(dialog);
}
else{
    gtk_widget_destroy(dialog);
}
}


void on_reception_clicked(GtkButton *button,gpointer data)
{
GtkWidget *dialog=gtk_file_chooser_dialog_new("Send to reception",NULL,
GTK_FILE_CHOOSER_ACTION_SAVE,
"cancel",GTK_RESPONSE_CANCEL,
"send",GTK_RESPONSE_ACCEPT,
NULL);

if(gtk_dialog_run(GTK_DIALOG(dialog))==GTK_RESPONSE_ACCEPT)
{
    char *filename=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

    g_free(filename);
    gtk_widget_destroy(dialog);
}

else{
    gtk_widget_destroy(dialog);
}
}

int main(int argc, char *argv[])
{

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    AppData app;
    app.port = port;

    GtkBuilder *builder;
    GtkWidget *window;

    gtk_init(&argc, &argv);

    builder=gtk_builder_new_from_file("doctor_UI.glade");
    window=GTK_WIDGET(gtk_builder_get_object(builder,"mainwindow"));
    gtk_window_set_default_size(GTK_WINDOW(window),800,600);


    treeview = GTK_WIDGET(
        gtk_builder_get_object(builder, "treeview")
    );
    gtk_widget_set_name(treeview, "file_tree");

    textview = GTK_WIDGET(
        gtk_builder_get_object(builder, "textview")
    );
    gtk_widget_set_name(textview, "editor");

     store = gtk_list_store_new(
        1,
        G_TYPE_STRING
    );

    gtk_tree_view_set_model(
        GTK_TREE_VIEW(treeview),
        GTK_TREE_MODEL(store)
    );

    /* Create TreeView column */

    GtkCellRenderer *renderer =
        gtk_cell_renderer_text_new();

    GtkTreeViewColumn *column =
        gtk_tree_view_column_new_with_attributes(
            "Name",
            renderer,
            "text",
            0,
            NULL
        );

    gtk_tree_view_append_column(
        GTK_TREE_VIEW(treeview),
        column
    );

    /* Double-click handler */

    g_signal_connect(
        treeview,
        "row-activated",
        G_CALLBACK(on_file_activated),
        NULL
    );

    /* Start in current directory */

    char *cwd = getcwd(NULL, 0);

    if (cwd != NULL)
    {
        load_directory(cwd);
        free(cwd);
    }

    GtkWidget *pharmacy = GTK_WIDGET(gtk_builder_get_object(builder,"pharmacy"));
    g_signal_connect(pharmacy, "clicked",G_CALLBACK(on_pharmacy_clicked),&app);


    GtkWidget *reception = GTK_WIDGET(gtk_builder_get_object(builder,"reception"));
    g_signal_connect(reception, "clicked",G_CALLBACK(on_reception_clicked),NULL);


    GtkWidget *back = GTK_WIDGET(gtk_builder_get_object(builder,"back"));
    g_signal_connect(back, "clicked",G_CALLBACK(go_back),NULL);

    gtk_widget_set_name(pharmacy, "pharmacy");
    gtk_widget_set_name(reception, "reception");
    gtk_widget_set_name(back, "back");

    GtkCssProvider*provider=gtk_css_provider_new();
    gtk_css_provider_load_from_path(provider,"doctor.css",NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
    GTK_STYLE_PROVIDER(provider),
    GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_widget_show_all(window);
    gtk_main();
    return 0;
}
