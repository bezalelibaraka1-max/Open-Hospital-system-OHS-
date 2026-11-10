#include<stdio.h>
#include<gtk/gtk.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<glib.h> //for g_malloc() and g_free()

#define USEFUL_SIZE 900
GtkBuilder * builder;
GtkWidget * window;
GtkWidget *stack;
GtkTreeView *treeview;
GtkTreeView *q_tree;
GtkTreeView *r_tree;
GtkListStore *store;
GtkListStore * q_store;
GtkListStore *r_store;
GtkTreeModel *model;
GtkTreeIter iter;
GtkComboBoxText *combo;
GtkComboBoxText *entry_combo;
GtkWidget *spin_button;

enum {
    COL_MED
};

typedef struct {
    
    GtkEntry* med_entry;
    GtkEntry* q_med;
    GtkEntry *entry;
}Appdata;


void on_window_show(GtkWidget *widget, gpointer data)
{
    FILE *fp = fopen("medicine.txt","r");

    if(fp==NULL)
    {
        perror("fopen");
        return;
    }
    char buffer[USEFUL_SIZE];
    while (fgets(buffer,sizeof(buffer),fp)!=NULL)
    {
        buffer[strcspn(buffer,"\n")] = '\0';

        //add text to the combobox
        gtk_combo_box_text_append_text(combo,buffer);
        gtk_combo_box_text_append_text(entry_combo,buffer);


        //insert each line into treeview's model
        gtk_list_store_append(store, &iter);

        gtk_list_store_set(
            store,
            &iter,
            0,
            buffer,
            -1
        );
    }
    fclose(fp);

    FILE*f = fopen("quantity.txt","r"); 
    
    //clear list to avoid duplication
    gtk_list_store_clear(q_store);
    if(fp==NULL)
    {
        perror("fopen");
        return;
    }
    char storage[USEFUL_SIZE];
    while(fgets(storage,sizeof(storage),f)!=NULL)
    {
        storage[strcspn(storage,"\n")] = '\0';
        char *tab = strchr(storage, '\t');
        if(!tab) continue; //skip malformed lines
        *tab = '\0';
        const char *name = storage;
        int qty = atoi(tab+1);

        gtk_list_store_append(q_store,&iter);
        gtk_list_store_set(q_store,&iter,0,storage,1,qty,-1);
    }
    fclose(f);
    
}

void on_one_clicked(GtkButton *button, gpointer userdata)
{
    GtkStack *stack = GTK_STACK(userdata);
    gtk_stack_set_visible_child_name(stack,"quantity");
}

void on_two_clicked(GtkButton *button, gpointer userdata)
{
    GtkStack *stack = GTK_STACK(userdata);
    gtk_stack_set_visible_child_name(stack,"report");
}

void on_back_one_clicked(GtkButton * button, gpointer userdata)
{
    GtkStack *stack = GTK_STACK(userdata);
    gtk_stack_set_visible_child_name(stack,"medicine");
}


void on_back_two_clicked(GtkButton * button, gpointer userdata)
{
    GtkStack *stack = GTK_STACK(userdata);
    gtk_stack_set_visible_child_name(stack,"quantity");
}

void on_add_medicine_clicked(GtkButton *button, gpointer userdata)
{
    Appdata *data = (Appdata *)userdata;
    const char *name = gtk_entry_get_text(data->med_entry);

    if (name == NULL || *name == '\0')
    {return;}

    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter,0,name,-1);
    gtk_entry_set_text(data->med_entry,"");
  
    char filename[USEFUL_SIZE] = "/home/spirex/Documents/GTK_development/Hospital_system/Pharmacy/medicine.txt";
     model =gtk_tree_view_get_model(treeview);
    FILE *file = fopen(filename, "w");

    if (file == NULL)
    {g_printerr("Could not open file for writing\n");
        /*g_free(filename);
        no freeing here because filename is array of characters not pointer from malloc*/ 
        return;
    }
     

    /* Start from the first row */
    gboolean valid = gtk_tree_model_get_iter_first(model, &iter);

    while(valid)
    {
        gchar *med_name;
        gtk_tree_model_get(model,&iter,COL_MED,&med_name,-1);

        fprintf(file,"%s\n",med_name);
        

         g_free(med_name);
        //move to next row
        valid = gtk_tree_model_iter_next(model,&iter);
    }
    fclose(file);
}

void on_insert_clicked(GtkButton *button, gpointer userdata)
{
    Appdata *data = (Appdata *)userdata;
    const char *quantity = gtk_entry_get_text(data->q_med);
    int res = atoi(quantity);

    gchar *med_type = gtk_combo_box_text_get_active_text(combo);

    if(res == 0)
    {return;}

    gtk_list_store_append(q_store,&iter);
    gtk_list_store_set(q_store,&iter,0,med_type,1,res,-1);
    g_free(med_type);

    gtk_entry_set_text(data->q_med,"");

     char filename[USEFUL_SIZE] = "/home/spirex/Documents/GTK_development/Hospital_system/Pharmacy/quantity.txt";
     model =gtk_tree_view_get_model(q_tree);
    FILE *file = fopen(filename, "w");

    if (file == NULL)
    {g_printerr("Could not open file for writing\n");
        return;
    }
     

    /* Start from the first row */
    gboolean valid = gtk_tree_model_get_iter_first(model, &iter);

    while(valid)
    {
        gchar *type;
        gint amount;
        gtk_tree_model_get(model,&iter,0,&type,1,&amount,-1);

        fprintf(file,"%s\t%d\n",type,amount);

         g_free(type); //only free strings not integer

        //move to next row
        valid = gtk_tree_model_iter_next(model,&iter);
    }
    fclose(file);

}
void on_okay_clicked(GtkButton *button, gpointer userdata)
{

    Appdata *data = (Appdata *)userdata;
    double value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button));
    const char *pharm_report = gtk_entry_get_text(data->entry);
    gchar *pharm_type = gtk_combo_box_text_get_active_text(entry_combo);

    //printf("%s\t%s\t%s");

    gtk_list_store_append(r_store,&iter);
    gtk_list_store_set(r_store,&iter,0,pharm_report,1,pharm_type,2,value,-1);
    g_free(pharm_type);

    gtk_entry_set_text(data->entry,"");

     char file_name[USEFUL_SIZE] = "/home/spirex/Documents/GTK_development/Hospital_system/Pharmacy/report.txt";
     model =gtk_tree_view_get_model(r_tree);
    FILE *file = fopen(file_name, "w");

    if (file == NULL)
    {g_printerr("Could not open file for writing\n");
        return;
    }
        
        /* Start from the first row */
    gboolean valid = gtk_tree_model_get_iter_first(model, &iter);

    while(valid)
    {
        gchar *r_med;
        gdouble r_amount;
        gchar *r_type;
        gtk_tree_model_get(model,&iter,0,&r_med,1,&r_type,2,&r_amount,-1);

        fprintf(file,"%s\t%s\t%d\n",r_med,r_type,r_amount);
        

         g_free(r_med);
         g_free(r_type);
        //move to next row
        valid = gtk_tree_model_iter_next(model,&iter);
    }
    fclose(file);

}

int main(int argc ,char *argv[])
{
    gtk_init(&argc,&argv);

    GtkWidget *one;
    GtkWidget *two;
    GtkWidget *back_one;
    GtkWidget *back_two;
    GtkWidget* add;
    GtkWidget *okay;
    GtkWidget* insert;

    builder = gtk_builder_new_from_file("pharmacy.glade");
    window = GTK_WIDGET(gtk_builder_get_object(builder,"mainwindow"));
    gtk_window_set_default_size(GTK_WINDOW(window),800,600);
    stack = GTK_WIDGET(gtk_builder_get_object(builder,"stack"));

    Appdata* data = g_malloc(sizeof(Appdata));

    one = GTK_WIDGET(gtk_builder_get_object(builder,"one"));
    two = GTK_WIDGET(gtk_builder_get_object(builder,"two"));
    back_one = GTK_WIDGET(gtk_builder_get_object(builder,"back_one"));
    back_two = GTK_WIDGET(gtk_builder_get_object(builder,"back_two"));

    data->med_entry = GTK_ENTRY(gtk_builder_get_object(builder, "add_med"));
    data->q_med = GTK_ENTRY(gtk_builder_get_object(builder, "q_med"));
    add = GTK_WIDGET(gtk_builder_get_object(builder, "add"));
    insert = GTK_WIDGET(gtk_builder_get_object(builder,"insert"));

    treeview = GTK_TREE_VIEW(gtk_builder_get_object(builder,"tree"));
    q_tree = GTK_TREE_VIEW(gtk_builder_get_object(builder,"q_tree"));
    r_tree = GTK_TREE_VIEW(gtk_builder_get_object(builder,"r_tree"));

    store = gtk_list_store_new(1,G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview),GTK_TREE_MODEL(store));

    q_store = gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_INT);
    gtk_tree_view_set_model(GTK_TREE_VIEW(q_tree),GTK_TREE_MODEL(q_store));

    r_store = gtk_list_store_new(3,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_DOUBLE);
    gtk_tree_view_set_model(GTK_TREE_VIEW(r_tree),GTK_TREE_MODEL(r_store));

    combo = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "dropdown"));
    entry_combo = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder,"entry_combo"));

    spin_button = GTK_WIDGET(gtk_builder_get_object(builder,"spin_button"));
    data->entry = GTK_ENTRY(gtk_builder_get_object(builder,"entry"));


    okay = GTK_WIDGET(gtk_builder_get_object(builder,"okay"));

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();

    GtkTreeViewColumn *med = gtk_tree_view_column_new_with_attributes("Medicine-Type",renderer,"text",0,NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(treeview),med);

    GtkCellRenderer *show = gtk_cell_renderer_text_new();

    GtkTreeViewColumn *med_type_show = gtk_tree_view_column_new_with_attributes("Medicine",show,"text",0,NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(q_tree),med_type_show);

    show = gtk_cell_renderer_text_new();
    
    GtkTreeViewColumn *quantity_show = gtk_tree_view_column_new_with_attributes("Quantity",show,"text",1,NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(q_tree),quantity_show);

    GtkCellRenderer *look = gtk_cell_renderer_text_new();

    GtkTreeViewColumn *r_look = gtk_tree_view_column_new_with_attributes("Patient",look,"text",0,NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(r_tree),r_look);

    GtkTreeViewColumn *r_look_again = gtk_tree_view_column_new_with_attributes("Type",look,"text",1,NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(r_tree),r_look_again);

    GtkTreeViewColumn *r_look_again_again = gtk_tree_view_column_new_with_attributes("Amount",look,"text",2,NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(r_tree),r_look_again_again);

    g_signal_connect(window,"show",G_CALLBACK(on_window_show),NULL);
    g_signal_connect(one,"clicked",G_CALLBACK(on_one_clicked),stack);
    g_signal_connect(two,"clicked",G_CALLBACK(on_two_clicked),stack);
    g_signal_connect(back_one,"clicked",G_CALLBACK(on_back_one_clicked),stack);
    g_signal_connect(back_two,"clicked",G_CALLBACK(on_back_two_clicked),stack);
    g_signal_connect(okay,"clicked",G_CALLBACK(on_okay_clicked),data);

    g_signal_connect(add,"clicked",G_CALLBACK(on_add_medicine_clicked),data);
    g_signal_connect(insert,"clicked",G_CALLBACK(on_insert_clicked),data);

    gtk_widget_show_all(window);
    gtk_main();
    return 0;

}