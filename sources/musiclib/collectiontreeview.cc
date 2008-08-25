        class CollectionTreeView
            :   public WidgetLoader<Gtk::TreeView>
        {
              typedef std::set<TreeIter>                IterSet;
              typedef std::map<gint64, TreeIter>        IdIterMap; 

            public:

              Glib::RefPtr<Gtk::TreeStore>          TreeStore;
              Glib::RefPtr<Gtk::TreeModelFilter>    TreeStoreFilter;

            private:

              CollectionColumnsT                    CollectionColumns;
              PAccess<MPX::Library>                 m_Lib;
              PAccess<MPX::Covers>                  m_Covers;
              MPX::Source::PlaybackSourceMusicLib&  m_MLib;

              IdIterMap                             m_CollectionIterMap;

              Cairo::RefPtr<Cairo::ImageSurface>    m_DiscDefault;
              Glib::RefPtr<Gdk::Pixbuf>             m_DiscDefault_Pixbuf;
              Glib::RefPtr<Gdk::Pixbuf>             m_Stars[6];

              boost::optional<gint64>               m_DragCollectionId;
              boost::optional<gint64>               m_DragTrackId;
              Gtk::TreePath                         m_PathButtonPress;

              bool                                  m_ButtonPressed;
              bool                                  m_Hand;
              bool                                  m_ShowNew;

              Gtk::Entry*                           m_FilterEntry;



              virtual void
              on_row_activated (const TreeModel::Path& path, TreeViewColumn* column)
              {
                TreeIter iter = TreeStore->get_iter (TreeStoreFilter->convert_path_to_child_path(path));
                if(path.get_depth() == ROW_ALBUM)
                {
                        gint64 id = (*iter)[CollectionColumns.Id];
                        m_MLib.play_album(id);
                }
                else
                {
                        gint64 id = (*iter)[CollectionColumns.TrackId];
                        IdV v;
                        v.push_back(id);
                        m_MLib.play_tracks(v);
                }
              }

              virtual void
              on_row_expanded (const TreeIter &iter_filter,
                               const TreePath &path) 
              {
#if 0
                TreeIter iter = TreeStoreFilter->convert_iter_to_child_iter(iter_filter);
                if(!(*iter)[CollectionColumns.HasTracks])
                {
                    GtkTreeIter children;
                    bool has_children = (gtk_tree_model_iter_children(GTK_TREE_MODEL(TreeStore->gobj()), &children, const_cast<GtkTreeIter*>(iter->gobj())));

                    SQL::RowV v;
                    m_Lib.get().getSQL(v, (boost::format("SELECT * FROM track_view WHERE album_j = %lld ORDER BY track;") % gint64((*iter)[CollectionColumns.Id])).str());

                    for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                    {
                        SQL::Row & r = *i;
                        TreeIter child = TreeStore->append(iter->children());
                        (*child)[CollectionColumns.TrackTitle] = get<std::string>(r["title"]);
                        (*child)[CollectionColumns.TrackArtist] = get<std::string>(r["artist"]);
                        (*child)[CollectionColumns.TrackNumber] = get<gint64>(r["track"]);
                        (*child)[CollectionColumns.TrackLength] = get<gint64>(r["time"]);
                        (*child)[CollectionColumns.TrackId] = get<gint64>(r["id"]);
                        (*child)[CollectionColumns.RowType] = ROW_TRACK; 
                    }

                    if(v.size())
                    {
                        (*iter)[CollectionColumns.HasTracks] = true;
                        if(has_children)
                        {
                            gtk_tree_store_remove(GTK_TREE_STORE(TreeStore->gobj()), &children);
                        } 
                        else
                            g_warning("%s:%d : No placeholder row present, state seems corrupted.", __FILE__, __LINE__);
                    }

                }
#endif
                scroll_to_row (path, 0.);
              }

              virtual void
              on_drag_data_get (const Glib::RefPtr<Gdk::DragContext>& context, SelectionData& selection_data, guint info, guint time)
              {
#if 0
                if(m_DragCollectionId)
                {
                    gint64 * id = new gint64(m_DragCollectionId.get());
                    selection_data.set("mpx-album", 8, reinterpret_cast<const guint8*>(id), 8);
                }
                else if(m_DragTrackId)
                {
                    gint64 * id = new gint64(m_DragTrackId.get());
                    selection_data.set("mpx-track", 8, reinterpret_cast<const guint8*>(id), 8);
                }

                m_DragMBID.reset();
                m_DragCollectionId.reset();
                m_DragTrackId.reset();
#endif
              }

              virtual void
              on_drag_begin (const Glib::RefPtr<Gdk::DragContext>& context) 
              {
#if 0
                if(m_DragCollectionId)
                {
                    if(m_DragMBID)
                    {
                        Cairo::RefPtr<Cairo::ImageSurface> CoverCairo;
                        m_Covers.get().fetch(m_DragMBID.get(), CoverCairo, COVER_SIZE_DEFAULT);
                        if(CoverCairo)
                        {
                            CoverCairo = Util::cairo_image_surface_round(CoverCairo, 21.3); 
                            Glib::RefPtr<Gdk::Pixbuf> CoverPixbuf = Util::cairo_image_surface_to_pixbuf(CoverCairo);
                            drag_source_set_icon(CoverPixbuf->scale_simple(128,128,Gdk::INTERP_BILINEAR));
                            return;
                        }
                    }

                    drag_source_set_icon(m_DiscDefault_Pixbuf->scale_simple(128,128,Gdk::INTERP_BILINEAR));
                }
                else
                {
                    Glib::RefPtr<Gdk::Pixmap> pix = create_row_drag_icon(m_PathButtonPress);
                    drag_source_set_icon(pix->get_colormap(), pix, Glib::RefPtr<Gdk::Bitmap>(0));
                }
#endif
              }

              virtual bool
              on_motion_notify_event (GdkEventMotion *event)
              {
#if 0
                int x_orig, y_orig;
                GdkModifierType state;

                if (event->is_hint)
                {
                  gdk_window_get_pointer (event->window, &x_orig, &y_orig, &state);
                }
                else
                {
                  x_orig = int (event->x);
                  y_orig = int (event->y);
                  state = GdkModifierType (event->state);
                }

                TreePath path;              
                TreeViewColumn *col;
                int cell_x, cell_y;
                if(get_path_at_pos (x_orig, y_orig, path, col, cell_x, cell_y))
                {
                    TreeIter iter = TreeStore->get_iter(TreeStoreFilter->convert_path_to_child_path(path));
                    CollectionRowType rt = (*iter)[CollectionColumns.RowType];
                    if( rt == ROW_ALBUM )
                    {
                            if(!g_atomic_int_get(&m_ButtonPressed))
                                return false;

                            if( (cell_x >= 102) && (cell_x <= 162) && (cell_y >= 65) && (cell_y <=78))
                            {
                                int rating = ((cell_x - 102)+7) / 12;
                                (*iter)[CollectionColumns.Rating] = rating;  
                                m_Lib.get().albumRated(m_DragCollectionId.get(), rating);
                                return true;
                            }
                    }
#if 0
                    else
                    if( rt == ROW_TRACK )
                    {
                        TreeViewColumn * col = get_column(0);
                        int start_x, width;
                        if( col->get_cell_position( *m_CellArtist, start_x, width ) )
                        {
                                if( cell_x >= start_x && cell_x <= (start_x+width)) 
                                {
                                    GdkCursor *cursor = gdk_cursor_new_from_name (gdk_display_get_default (), "hand2");
                                    gdk_window_set_cursor (GTK_WIDGET (gobj())->window, cursor);
                                    m_Hand = true;
                                }
                                else
                                if( m_Hand )
                                {
                                    GdkCursor *cursor = gdk_cursor_new_from_name (gdk_display_get_default (), "left_ptr");
                                    gdk_window_set_cursor (GTK_WIDGET (gobj())->window, cursor);
                                    m_Hand = false;
                                }
                        }
                        else
                                g_message("Cell not in column.");
                    }
#endif
                }

                return false;
#endif
             } 

              virtual bool
              on_button_release_event (GdkEventButton* event)
              {
#if 0
                g_atomic_int_set(&m_ButtonPressed, 0);
                return false;
#endif
              }

              virtual bool
              on_button_press_event (GdkEventButton* event)
              {
#if 0
                int cell_x, cell_y ;
                TreeViewColumn *col ;

                if(get_path_at_pos (event->x, event->y, m_PathButtonPress, col, cell_x, cell_y))
                {
                    TreeIter iter = TreeStore->get_iter(TreeStoreFilter->convert_path_to_child_path(m_PathButtonPress));
                    if(m_PathButtonPress.get_depth() == ROW_ALBUM)
                    {
                        m_DragMBID = (*iter)[CollectionColumns.MBID];
                        m_DragCollectionId = (*iter)[CollectionColumns.Id];
                        m_DragTrackId.reset(); 
                
                        g_atomic_int_set(&m_ButtonPressed, 1);

                        if( (cell_x >= 102) && (cell_x <= 162) && (cell_y >= 65) && (cell_y <=78))
                        {
                            int rating = ((cell_x - 102)+7) / 12;
                            (*iter)[CollectionColumns.Rating] = rating;  
                            m_Lib.get().albumRated(m_DragCollectionId.get(), rating);
                        }
                    }
                    else
                    if(m_PathButtonPress.get_depth() == ROW_TRACK)
                    {
                        m_DragMBID.reset(); 
                        m_DragCollectionId.reset();
                        m_DragTrackId = (*iter)[CollectionColumns.TrackId];
                    }
                }
                TreeView::on_button_press_event(event);
                return false;
#endif
              }

              void
              on_got_cover(const Glib::ustring& mbid)
              {
#if 0
                Cairo::RefPtr<Cairo::ImageSurface> Cover;
                m_Covers.get().fetch(mbid, Cover, COVER_SIZE_ALBUM);
                Util::cairo_image_surface_border(Cover, 2.);
                Cover = Util::cairo_image_surface_round(Cover, 6.);
                IterSet & set = m_MBIDIterMap[mbid];
                for(IterSet::iterator i = set.begin(); i != set.end(); ++i)
                {
                    (*(*i))[CollectionColumns.Image] = Cover;
                }
#endif
              }

              void
              place_album (SQL::Row & r, gint64 id)
              {
#if 0
                TreeIter iter = TreeStore->append();
                m_CollectionIterMap.insert(std::make_pair(id, iter));
                TreeStore->append(iter->children()); //create dummy/placeholder row for tracks

                (*iter)[CollectionColumns.RowType] = ROW_ALBUM; 
                (*iter)[CollectionColumns.HasTracks] = false; 
                (*iter)[CollectionColumns.NewCollection] = get<gint64>(r["album_new"]);
                (*iter)[CollectionColumns.Image] = m_DiscDefault; 
                (*iter)[CollectionColumns.Id] = id; 

                if(r.count("album_rating"))
                {
                    gint64 rating = get<gint64>(r["album_rating"]);
                    (*iter)[CollectionColumns.Rating] = rating;
                }

                gint64 idate = 0;
                if(r.count("album_insert_date"))
                {
                    idate = get<gint64>(r["album_insert_date"]);
                    (*iter)[CollectionColumns.InsertDate] = idate;
                }

                std::string asin;
                if(r.count("amazon_asin"))
                {
                    asin = get<std::string>(r["amazon_asin"]);
                }

                if(r.count("mb_album_id"))
                {
                    std::string mbid = get<std::string>(r["mb_album_id"]);

                    IterSet & s = m_MBIDIterMap[mbid];
                    s.insert(iter);

                    (*iter)[CollectionColumns.MBID] = mbid; 
                }

                std::string date; 
                if(r.count("mb_release_date"))
                {
                    date = get<std::string>(r["mb_release_date"]);
                    if(date.size())
                    {
                        gint64 date_int;
                        sscanf(date.c_str(), "%04lld", &date_int);
                        (*iter)[CollectionColumns.Date] = date_int; 
                        date = date.substr(0,4) + ", ";
                    }
                }
                else
                    (*iter)[CollectionColumns.Date] = 0; 

                std::string ArtistSort;
                if(r.find("album_artist_sortname") != r.end())
                    ArtistSort = get<std::string>(r["album_artist_sortname"]);
                else
                    ArtistSort = get<std::string>(r["album_artist"]);

                (*iter)[CollectionColumns.Text] =
                    (boost::format("<span size='12000'><b>%2%</b></span>\n<span size='12000'>%1%</span>\n<span size='9000'>%3%Added: %4%</span>")
                        % Markup::escape_text(get<std::string>(r["album"])).c_str()
                        % Markup::escape_text(ArtistSort).c_str()
                        % date
                        % get_timestr_from_time_t(idate)
                    ).str();

                (*iter)[CollectionColumns.CollectionSort] = ustring(get<std::string>(r["album"])).collate_key();
                (*iter)[CollectionColumns.ArtistSort] = ustring(ArtistSort).collate_key();
#endif
              } 
   
              void
              album_list_load ()
              {
#if 0
                g_message("%s: Reloading.", G_STRLOC);

                TreeStore->clear ();
                m_MBIDIterMap.clear();
                m_CollectionIterMap.clear();

                SQL::RowV v;
                m_Lib.get().getSQL(v, "SELECT * FROM album JOIN album_artist ON album.album_artist_j = album_artist.id;");

                for(SQL::RowV::iterator i = v.begin(); i != v.end(); ++i)
                {
                    SQL::Row & r = *i; 
                    place_album(r, get<gint64>(r["id"]));
                    m_Covers.get().cache(get<std::string>(r["mb_album_id"]));
                }
#endif
              }

              void
              on_new_album(gint64 id)
              {
#if 0
                SQL::RowV v;
                m_Lib.get().getSQL(v, (boost::format("SELECT * FROM album JOIN album_artist ON album.album_artist_j = album_artist.id WHERE album.id = %lld;") % id).str());

                g_return_if_fail(!v.empty());

                SQL::Row & r = v[0];

                place_album (r, id); 
#endif
              }

              void
              on_new_track(Track & track, gint64 album_id, gint64 artist_id)
              {
#if 0
                if(m_CollectionIterMap.count(album_id))
                {
                    TreeIter iter = m_CollectionIterMap[album_id];
                    if (((*iter)[CollectionColumns.HasTracks]))
                    {
                        TreeIter child = TreeStore->append(iter->children());
                        if(track[ATTRIBUTE_TITLE])
                            (*child)[CollectionColumns.TrackTitle] = get<std::string>(track[ATTRIBUTE_TITLE].get());
                        if(track[ATTRIBUTE_ARTIST])
                            (*child)[CollectionColumns.TrackArtist] = get<std::string>(track[ATTRIBUTE_ARTIST].get());
                        if(track[ATTRIBUTE_MB_ARTIST_ID])
                            (*child)[CollectionColumns.TrackArtistMBID] = get<std::string>(track[ATTRIBUTE_MB_ARTIST_ID].get());
                        if(track[ATTRIBUTE_TRACK])
                            (*child)[CollectionColumns.TrackNumber] = get<gint64>(track[ATTRIBUTE_TRACK].get());
                        if(track[ATTRIBUTE_TIME])
                            (*child)[CollectionColumns.TrackLength] = get<gint64>(track[ATTRIBUTE_TIME].get());

                        (*child)[CollectionColumns.TrackId] = get<gint64>(track[ATTRIBUTE_MPX_TRACK_ID].get());
                        (*child)[CollectionColumns.RowType] = ROW_TRACK; 
                    }
                }
                else
                    g_warning("%s: Got new track without associated album! Consistency error!", G_STRLOC);
#endif
              }

              int
              slotSortRating(const TreeIter& iter_a, const TreeIter& iter_b)
              {
#if 0
                CollectionRowType rt_a = (*iter_a)[CollectionColumns.RowType];
                CollectionRowType rt_b = (*iter_b)[CollectionColumns.RowType];

                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                {
                  gint64 alb_a = (*iter_a)[CollectionColumns.Rating];
                  gint64 alb_b = (*iter_b)[CollectionColumns.Rating];

                  return alb_b - alb_a;
                }
                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                {
                  gint64 trk_a = (*iter_a)[CollectionColumns.TrackNumber];
                  gint64 trk_b = (*iter_b)[CollectionColumns.TrackNumber];

                  return trk_a - trk_b;
                }

                return 0;
#endif
              }

              int
              slotSortAlpha(const TreeIter& iter_a, const TreeIter& iter_b)
              {
#if 0
                CollectionRowType rt_a = (*iter_a)[CollectionColumns.RowType];
                CollectionRowType rt_b = (*iter_b)[CollectionColumns.RowType];

                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                {
                  gint64 alb_a = (*iter_a)[CollectionColumns.Date];
                  gint64 alb_b = (*iter_b)[CollectionColumns.Date];
                  std::string art_a = (*iter_a)[CollectionColumns.ArtistSort];
                  std::string art_b = (*iter_b)[CollectionColumns.ArtistSort];

                  if(art_a != art_b)
                      return art_a.compare(art_b);

                  return alb_a - alb_b;
                }
                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                {
                  gint64 trk_a = (*iter_a)[CollectionColumns.TrackNumber];
                  gint64 trk_b = (*iter_b)[CollectionColumns.TrackNumber];

                  return trk_a - trk_b;
                }

                return 0;
#endif
              }

              int
              slotSortDate(const TreeIter& iter_a, const TreeIter& iter_b)
              {
#if 0
                CollectionRowType rt_a = (*iter_a)[CollectionColumns.RowType];
                CollectionRowType rt_b = (*iter_b)[CollectionColumns.RowType];

                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                {
                  gint64 alb_a = (*iter_a)[CollectionColumns.InsertDate];
                  gint64 alb_b = (*iter_b)[CollectionColumns.InsertDate];

                  return alb_b - alb_a;
                }
                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                {
                  gint64 trk_a = (*iter_a)[CollectionColumns.TrackNumber];
                  gint64 trk_b = (*iter_b)[CollectionColumns.TrackNumber];

                  return trk_a - trk_b;
                }

                return 0;
#endif
              }

              int
              slotSortStrictAlpha(const TreeIter& iter_a, const TreeIter& iter_b)
              {
#if 0
                CollectionRowType rt_a = (*iter_a)[CollectionColumns.RowType];
                CollectionRowType rt_b = (*iter_b)[CollectionColumns.RowType];

                if((rt_a == ROW_ALBUM) && (rt_b == ROW_ALBUM))
                {
                  std::string alb_a = (*iter_a)[CollectionColumns.CollectionSort];
                  std::string alb_b = (*iter_b)[CollectionColumns.CollectionSort];
                  std::string art_a = (*iter_a)[CollectionColumns.ArtistSort];
                  std::string art_b = (*iter_b)[CollectionColumns.ArtistSort];

                  if(art_a != art_b)
                      return art_a.compare(art_b);

                  return alb_a.compare(alb_b);
                }
                else if((rt_a == ROW_TRACK) && (rt_b == ROW_TRACK))
                {
                  gint64 trk_a = (*iter_a)[CollectionColumns.TrackNumber];
                  gint64 trk_b = (*iter_b)[CollectionColumns.TrackNumber];

                  return trk_a - trk_b;
                }

                return 0;
#endif
              }

              void
              cellDataFuncCover (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
#if 0
                TreePath path (iter);
                CellRendererCairoSurface *cell = dynamic_cast<CellRendererCairoSurface*>(basecell);
                if(path.get_depth() == ROW_ALBUM)
                {
                    cell->property_visible() = true;
                    cell->property_surface() = (*iter)[CollectionColumns.Image]; 
                }
                else
                {
                    cell->property_visible() = false;
                }
#endif
              }

              void
              cellDataFuncText1 (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
#if 0
                TreePath path (iter);
                CellRendererVBox *cvbox = dynamic_cast<CellRendererVBox*>(basecell);
                CellRendererText *cell1 = dynamic_cast<CellRendererText*>(cvbox->property_renderer1().get_value());
                CellRendererPixbuf *cell2 = dynamic_cast<CellRendererPixbuf*>(cvbox->property_renderer2().get_value());
                if(path.get_depth() == ROW_ALBUM)
                {
                    cvbox->property_visible() = true; 

                    if(cell1)
                        cell1->property_markup() = (*iter)[CollectionColumns.Text]; 
                    if(cell2)
                    {
                        gint64 i = ((*iter)[CollectionColumns.Rating]);
                        g_return_if_fail((i >= 0) && (i <= 5));
                        cell2->property_pixbuf() = m_Stars[i];
                    }
                }
                else
                {
                    cvbox->property_visible() = false; 
                }
#endif
              }

              void
              cellDataFuncText2 (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
#if 0
                TreePath path (iter);
                CellRendererCount *cell = dynamic_cast<CellRendererCount*>(basecell);
                if(path.get_depth() == ROW_TRACK)
                {
                    cell->property_visible() = true; 
                    cell->property_text() = (boost::format("%lld") % (*iter)[CollectionColumns.TrackNumber]).str();
                }
                else
                {
                    cell->property_visible() = false; 
                }
#endif
              }

              void
              cellDataFuncText3 (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
#if 0
                TreePath path (iter);
                CellRendererText *cell = dynamic_cast<CellRendererText*>(basecell);
                if(path.get_depth() == ROW_TRACK)
                {
                    cell->property_visible() = true; 
                    cell->property_markup() = Markup::escape_text((*iter)[CollectionColumns.TrackTitle]);
                }
                else
                {
                    cell->property_visible() = false; 
                }
#endif
              }

              void
              cellDataFuncText4 (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
#if 0
                TreePath path (iter);
                CellRendererText *cell = dynamic_cast<CellRendererText*>(basecell);
                if(path.get_depth() == ROW_TRACK)
                {
                    cell->property_visible() = true; 
                    gint64 Length = (*iter)[CollectionColumns.TrackLength];
                    cell->property_text() = (boost::format ("%d:%02d") % (Length / 60) % (Length % 60)).str();
                }
                else
                {
                    cell->property_visible() = false; 
                }
#endif
              }

              void
              cellDataFuncText5 (CellRenderer * basecell, TreeModel::iterator const &iter)
              {
#if 0
                TreePath path (iter);
                CellRendererText *cell = dynamic_cast<CellRendererText*>(basecell);
                if(path.get_depth() == ROW_TRACK)
                {
                    cell->property_visible() = true; 
                    cell->property_markup() = (boost::format (_("<small>by</small> %s")) % Markup::escape_text((*iter)[CollectionColumns.TrackArtist])).str();
                }
                else
                {
                    cell->property_visible() = false; 
                }
#endif
              }

              static void
              rb_sourcelist_expander_cell_data_func (GtkTreeViewColumn *column,
                                     GtkCellRenderer   *cell,
                                     GtkTreeModel      *model,
                                     GtkTreeIter       *iter,
                                     gpointer           data) 
              {
#if 0
                  if (gtk_tree_model_iter_has_child (model, iter))
                  {
                      GtkTreePath *path;
                      gboolean     row_expanded;

                      path = gtk_tree_model_get_path (model, iter);
                      row_expanded = gtk_tree_view_row_expanded (GTK_TREE_VIEW (column->tree_view), path);
                      gtk_tree_path_free (path);

                      g_object_set (cell,
                                "visible", TRUE,
                                "expander-style", row_expanded ? GTK_EXPANDER_EXPANDED : GTK_EXPANDER_COLLAPSED,
                                NULL);
                  } else {
                      g_object_set (cell, "visible", FALSE, NULL);
                  }
#endif
              }

              bool
              albumVisibleFunc (TreeIter const& iter)
              {
#if 0
                  CollectionRowType rt = (*iter)[CollectionColumns.RowType];

                  if( m_ShowNew && rt == ROW_ALBUM ) 
                  {
                    bool visible = (*iter)[CollectionColumns.NewCollection];
                    if( !visible ) 
                        return false;
                  } 

                  ustring filter (ustring (m_FilterEntry->get_text()).lowercase());

                  TreePath path (TreeStore->get_path(iter));

                  if( filter.empty() || path.get_depth() > 1)
                    return true;
                  else
                    return (Util::match_keys (ustring((*iter)[CollectionColumns.Text]).lowercase(), filter)); 
#endif
              }

            public:

              void
              go_to_album(gint64 id)
              {
#if 0
                if(m_CollectionIterMap.count(id))
                {
                    TreeIter iter = m_CollectionIterMap.find(id)->second;
                    scroll_to_row (TreeStore->get_path(iter), 0.);
                }
#endif
              }

              void
              set_new_albums_state (int state)
              {
#if 0
                 m_ShowNew = state;
                 TreeStoreFilter->refilter();
#endif
              }

              CollectionTreeView(
                        Glib::RefPtr<Gnome::Glade::Xml> const& xml,    
                        PAccess<MPX::Library> const& lib,
                        PAccess<MPX::Covers> const& amzn,
                        MPX::Source::PlaybackSourceMusicLib & mlib
              )
              : WidgetLoader<Gtk::TreeView>(xml,"source-musiclib-treeview-collections")
              , m_Lib(lib)
              , m_Covers(amzn)
              , m_MLib(mlib)
              , m_ButtonPressed(false)
              , m_Hand(false)
              , m_ShowNew(false)
              {
                for(int n = 0; n < 6; ++n)
                    m_Stars[n] = Gdk::Pixbuf::create_from_file(build_filename(build_filename(DATA_DIR,"images"),
                        (boost::format("stars%d.png") % n).str()));

                m_Lib.get().signal_new_album().connect( sigc::mem_fun( *this, &CollectionTreeView::on_new_album ));
                m_Lib.get().signal_new_track().connect( sigc::mem_fun( *this, &CollectionTreeView::on_new_track ));
                m_Lib.get().signal_reload().connect( sigc::mem_fun( *this, &CollectionTreeView::album_list_load ));
                m_Covers.get().signal_got_cover().connect( sigc::mem_fun( *this, &CollectionTreeView::on_got_cover ));

                xml->get_widget("collections-filter-entry", m_FilterEntry);

                set_show_expanders( false );
                set_level_indentation( 32 );

                TreeViewColumn * col = manage (new TreeViewColumn());

                GtkCellRenderer * renderer = gossip_cell_renderer_expander_new ();
                gtk_tree_view_column_pack_start (col->gobj(), renderer, FALSE);
                gtk_tree_view_column_set_cell_data_func (col->gobj(),
                                     renderer,
                                     GtkTreeCellDataFunc(rb_sourcelist_expander_cell_data_func),
                                     this,
                                     NULL);

                CellRendererCairoSurface * cellcairo = manage (new CellRendererCairoSurface);
                col->pack_start(*cellcairo, false);
                col->set_cell_data_func(*cellcairo, sigc::mem_fun( *this, &CollectionTreeView::cellDataFuncCover ));
                cellcairo->property_xpad() = 4;
                cellcairo->property_ypad() = 4;
                cellcairo->property_yalign() = 0.;
                cellcairo->property_xalign() = 0.;

                CellRendererVBox *cvbox = manage (new CellRendererVBox);

                CellRendererText *celltext = manage (new CellRendererText);
                celltext->property_yalign() = 0.;
                celltext->property_ypad() = 4;
                celltext->property_height() = 52;
                celltext->property_ellipsize() = Pango::ELLIPSIZE_MIDDLE;
                cvbox->property_renderer1() = celltext;

                CellRendererPixbuf *cellpixbuf = manage (new CellRendererPixbuf);
                cellpixbuf->property_xalign() = 0.;
                cellpixbuf->property_ypad() = 2;
                cellpixbuf->property_xpad() = 2;
                cvbox->property_renderer2() = cellpixbuf;

                col->pack_start(*cvbox, true);
                col->set_cell_data_func(*cvbox, sigc::mem_fun( *this, &CollectionTreeView::cellDataFuncText1 ));

                CellRendererCount *cellcount = manage (new CellRendererCount);
                col->pack_start(*cellcount, false);
                col->set_cell_data_func(*cellcount, sigc::mem_fun( *this, &CollectionTreeView::cellDataFuncText2 ));
                cellcount->property_box() = BOX_NORMAL;

                celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);
                col->set_cell_data_func(*celltext, sigc::mem_fun( *this, &CollectionTreeView::cellDataFuncText3 ));

                celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);
                col->set_cell_data_func(*celltext, sigc::mem_fun( *this, &CollectionTreeView::cellDataFuncText4 ));

                celltext = manage (new CellRendererText);
                col->pack_start(*celltext, false);
                celltext->property_xalign() = 0.;
                celltext->property_xpad() = 2;
                col->set_cell_data_func(*celltext, sigc::mem_fun( *this, &CollectionTreeView::cellDataFuncText5 ));

                append_column(*col);

                TreeStore = Gtk::TreeStore::create(CollectionColumns);
                TreeStoreFilter = Gtk::TreeModelFilter::create(TreeStore);
                TreeStoreFilter->set_visible_func (sigc::mem_fun (*this, &CollectionTreeView::albumVisibleFunc));
                m_FilterEntry->signal_changed().connect (sigc::mem_fun (TreeStoreFilter.operator->(), &TreeModelFilter::refilter));

                set_model(TreeStoreFilter);
                TreeStore->set_sort_func(0 , sigc::mem_fun( *this, &CollectionTreeView::slotSortAlpha ));
                TreeStore->set_sort_func(1 , sigc::mem_fun( *this, &CollectionTreeView::slotSortDate ));
                TreeStore->set_sort_func(2 , sigc::mem_fun( *this, &CollectionTreeView::slotSortRating ));
                TreeStore->set_sort_func(3 , sigc::mem_fun( *this, &CollectionTreeView::slotSortStrictAlpha ));
                TreeStore->set_sort_column(0, Gtk::SORT_ASCENDING);

                m_DiscDefault_Pixbuf = Gdk::Pixbuf::create_from_file(build_filename(DATA_DIR, build_filename("images","disc.png")));
                m_DiscDefault = Util::cairo_image_surface_from_pixbuf(m_DiscDefault_Pixbuf->scale_simple(72,72,Gdk::INTERP_BILINEAR));

                album_list_load ();

                std::vector<TargetEntry> Entries;
                Entries.push_back(TargetEntry("mpx-collection", TARGET_SAME_APP, 0x82));
                Entries.push_back(TargetEntry("mpx-track", TARGET_SAME_APP, 0x81));
                drag_source_set(Entries); 
              }
        };

