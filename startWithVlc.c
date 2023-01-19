#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <stdlib.h>
#include <vlc/vlc.h>
libvlc_instance_t * inst;
libvlc_media_player_t *mp;
libvlc_media_t *m;
GtkWidget *window,*player_widget;


void player_widget_on_realize(GtkWidget *widget, gpointer data);

void button_clicked(GtkWidget *widget, gpointer data) {
    libvlc_release(inst);
    inst = libvlc_new (0,NULL);
    libvlc_media_player_release(mp);
    system("streamer -c /dev/video0 -b 16 -o outfile.jpeg");    
    mp = libvlc_media_player_new_from_media (m);
    libvlc_media_player_play(mp);
    libvlc_media_player_set_xwindow(mp, GDK_WINDOW_XID(gtk_widget_get_window(widget)));
}

void configure_callback(GtkWidget *widget, GdkEvent *event, gpointer data){
    libvlc_release(inst);
    inst = libvlc_new (0,NULL);
    libvlc_media_player_release(mp);
    mp = libvlc_media_player_new_from_media (m);
    libvlc_media_player_play(mp);
    libvlc_media_player_set_xwindow(mp, GDK_WINDOW_XID(gtk_widget_get_window(widget)));

}

void player_widget_on_realize(GtkWidget *widget, gpointer data) {
    inst = libvlc_new (0,NULL);
    m = libvlc_media_new_location (inst, "v4l2:///dev/video0");
    mp = libvlc_media_player_new_from_media (m);
    libvlc_media_player_play(mp);
    libvlc_media_player_set_xwindow(mp, GDK_WINDOW_XID(gtk_widget_get_window(widget)));
    g_signal_connect(G_OBJECT(widget), "configure-event", G_CALLBACK(configure_callback), NULL);    
}



static void activate (GtkApplication* app,
          gpointer        user_data)
{
    GtkWidget  *vbox, *button;
    const gchar *button_label = "teste";
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0); 
    window = gtk_application_window_new (app);
    button = gtk_button_new_with_label(button_label);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_window_set_title (GTK_WINDOW (window), "Policorp");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size (GTK_WINDOW (window), 800,600);
    gtk_widget_add_events(GTK_WIDGET(window), GDK_CONFIGURE);  
    player_widget = gtk_drawing_area_new();
    gtk_box_pack_start(GTK_BOX(vbox), player_widget, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(vbox), button, FALSE, TRUE, 0);
    g_signal_connect(G_OBJECT(button), "clicked",  G_CALLBACK(button_clicked), NULL);    
    g_signal_connect(G_OBJECT(player_widget), "realize", G_CALLBACK(player_widget_on_realize), NULL);    
    gtk_widget_show_all (window);
   
}


int
main (int    argc,
      char **argv)
{   
    GtkApplication  *app;  
    int status;
    app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;

}
