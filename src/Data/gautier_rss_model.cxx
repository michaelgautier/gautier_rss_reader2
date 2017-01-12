#include <gautier_rss_model.hxx>

using type_list_size = std::vector<void*>::size_type;

static bool 
        _query_values_translator_output_enabled = false,
        _sql_trace_enabled = false,
        _rows_affected_output_enabled = false,
        _issued_sql_output_enabled = false,
        _sql_trace_active = false;

gautier::rss_model::rss_model() {
        _comment_marker = '#';
        _feed_config_line_sep = '\t';

        _list_reserve_size = 200;

        _element_name_item = "item";
        _rss_database_name = "rss_feeds_info.db";

        _element_names = {"title", "link", "description", "pubdate"};
        _table_names = {"rss_feed_source", "rss_feed_data", "rss_feed_data_staging"};

        _empty_param_set = {
          std::tuple<std::string, std::string, parameter_data_type>("", "", parameter_data_type::none)
        };

        return;
}

static void 
db_connection_guard_finalize(sqlite3* obj);

static bool 
translate_sql_result(std::shared_ptr<std::vector<std::map<std::string, std::string>>> query_values, sqlite3_stmt* sql_stmt);

static int 
translate_sql_result(void* user_defined_data, int column_count, char** column_values, char** column_names);

static int
switch_letter_case (const char& in_char);


static void 
enable_op_sql_trace(sqlite3** db_connection);

static void 
trace_sql_op(void*, const char*);

static void 
enable_op_sql_autolog();

static void 
log_sql_op_event(void *pArg, int iErrCode, const char *zMsg);

static void 
output_op_sql_error_message(char** error_message, const int& line_number);

static void 
output_op_sql_error_message(sqlite3** db_connection, const int& line_number);


//Public, API.

//Take a file with name/value pairs and converts them into a 
//	a data structure by the name of std::map<std::string, unit_type_rss_source>.
//	The collect_feed_items_from_rss function is the main function and it needs 
//		an input data structure of type, std::map<std::string, unit_type_rss_source>.
//This function provides that output that is the input into collect_feed_items_from_rss.
//This function is optional and is one form of a solution for deriving the output.
//This function deals with a plain-text file. The file format has 
//	the name of the feed in column 1 and the rss feed url in column 2. 
//	Each column is separated by a tab.
//This version reads all lines into a data structure to be fed into collect_feed_items_from_rss function.
//Since the number of lines will not exceed a 100 or 200 lines, this approach is acceptable. 
//An alternative version that reads each line into a database table as a line is encountered
//	was considered for version 1, but is deferred until such time such an optimization is preferred.
//The current approach has more flexibility in that it does not initially depend on anything else.
//That is, the top-level functions can be adapted to data sources of any type since they are not 
//	tightly coupled to any specific data format other than a plain-text file.

void //*This function, or a function like it, has to be called first.
gautier::rss_model::load_feeds_source_list(const std::string& feeds_list_file_name, std::map<std::string, gautier::rss_model::unit_type_rss_source>& feed_sources)
{
  std::map<std::string, gautier::rss_model::unit_type_rss_source> tmp_feed_sources;

  if(!feeds_list_file_name.empty())
  {
    std::ifstream feeds_file;

    feeds_file.open(feeds_list_file_name);

    if(feeds_file)
    {
      while(feeds_file.good() && !feeds_file.eof())
      {
        std::string line_data = "";

        std::getline(feeds_file, line_data);

        if(!line_data.empty() && line_data.front() != _comment_marker)
        {
          //Consider a validation in the future for URL and name.
          auto tab_pos = line_data.find(_feed_config_line_sep);

          if(tab_pos != std::string::npos)
          {
            gautier::rss_model::unit_type_rss_source rss_source;

            rss_source.name = std::string(line_data, 0, tab_pos);
            rss_source.url = std::string(line_data, tab_pos+1, std::string::npos);

            tmp_feed_sources[rss_source.name] = rss_source;
          }
        }
      }
    }
  }

  load_feeds_source_list(tmp_feed_sources);

  feed_sources = tmp_feed_sources;

  return;
}

void 
gautier::rss_model::load_feeds_source_list(std::map<std::string, gautier::rss_model::unit_type_rss_source>& feed_sources)
{
  std::map<std::string, gautier::rss_model::unit_type_rss_source> tmp_feed_sources;

  filter_feeds_source(feed_sources, tmp_feed_sources);

  feed_sources = tmp_feed_sources;

  return;
}

//Main logic.
//Ties together the process of pulling in rss feed data (in XML format) 
//	into a data structure named std::map<std::string, std::vector<std::map<std::string, std::string>>> that is used 
//	by other processes to present rss headline and web address information.
void 
gautier::rss_model::collect_feeds(const std::map<std::string, gautier::rss_model::unit_type_rss_source>& feed_sources)
{
  if(!feed_sources.empty())
  {
    std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>> rss_feed_items;

    collect_feed_items_from_rss(feed_sources, rss_feed_items);

    save_feeds(rss_feed_items);
  }

  return;
}

void 
gautier::rss_model::collect_feeds(const std::map<std::string, gautier::rss_model::unit_type_rss_source>& feed_sources, std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items)
{
  collect_feeds(feed_sources);

  load_feeds(rss_feed_items);

  return;
}

void 
gautier::rss_model::load_feeds(std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items)
{
  std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>> tmp_rss_feed_items;

  if(!rss_feed_items.empty())
  {
    rss_feed_items.clear();
  }

  sqlite3* db_connection = nullptr;

  db_check_database_exist(&db_connection);

  if(db_connection)
  {
    std::shared_ptr<sqlite3> db_connection_guard(db_connection, db_connection_guard_finalize);

    //optimization
    //preallocate feed items in contiguous groups.
    {
      std::string
          sql_text =
          "SELECT \
          fs.name, \
          COUNT(fd.id) AS total_sub_items \
          FROM rss_feed_source AS fs INNER JOIN \
          rss_feed_data AS fd ON fs.id = fd.rss_feed_source_id \
                                         GROUP BY fs.name \
                                         ORDER BY fs.name;\
      ";

      std::shared_ptr<std::vector<std::map<std::string, std::string>>> query_values;
      query_values.reset(new std::vector<std::map<std::string, std::string>>);

      char* error_message = 0;

      const auto sqlite_result =
          sqlite3_exec(db_connection, sql_text.data(), translate_sql_result, query_values.get(), &error_message);

      if(sqlite_result == SQLITE_OK)
      {
        if(query_values)
        {
          for(auto& row_of_data : *query_values)
          {
            const std::string feed_name = row_of_data["name"];
            const type_list_size item_count = std::stoul(row_of_data["total_sub_items"]);

            auto feed_items = &(tmp_rss_feed_items[feed_name]);

            feed_items->reserve(item_count);
          }
        }
      }
      else
      {
        output_op_sql_error_message(&error_message, __LINE__);
      }
    }

    //load the feed detail.
    {
      std::string sql_text =
          "SELECT \
          fs.name AS feed_name, \
          fd.pub_date, \
          fd.title, \
          fd.link, \
          fd.description \
          FROM rss_feed_source AS fs INNER JOIN \
          rss_feed_data AS fd ON fs.id = fd.rss_feed_source_id \
                                         ORDER BY \
                                         fs.name, \
          fd.pub_date, \
          fd.title;\
      ";

      std::shared_ptr<std::vector<std::map<std::string, std::string>>> query_values;
      query_values.reset(new std::vector<std::map<std::string, std::string>>);

      char* error_message = 0;

      const auto sqlite_result = sqlite3_exec(db_connection, sql_text.data(), translate_sql_result, query_values.get(), &error_message);

      if(sqlite_result == SQLITE_OK)
      {
        if(query_values && !query_values->empty())
        {
          for(auto& row_of_data : *query_values)
          {
            const std::string
                feed_name = row_of_data["feed_name"];

            gautier::rss_model::unit_type_rss_item feed_item;

            make_feed_item(row_of_data, feed_item);

            tmp_rss_feed_items[feed_name].push_back(feed_item);
          }
        }
      }
      else
      {
        output_op_sql_error_message(&error_message, __LINE__);
      }
    }
  }

  rss_feed_items = tmp_rss_feed_items;

  return;
}

void 
gautier::rss_model::load_feed(const gautier::rss_model::unit_type_rss_source& feed_source, std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items)
{
  std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>> tmp_rss_feed_items;

  if(!rss_feed_items.empty())
  {
    rss_feed_items.clear();
  }

  sqlite3* db_connection = nullptr;

  db_check_database_exist(&db_connection);

  if(db_connection)
  {
    std::shared_ptr<sqlite3> db_connection_guard(db_connection, db_connection_guard_finalize);

    //load the feed detail.
    {
      std::string sql_text{};

      std::vector<std::tuple<std::string, std::string, parameter_data_type>> parameter_values;

      if(feed_source.id > 0)
      {
        sql_text =
            "SELECT \
            fs.name AS feed_name, \
            fd.pub_date, \
            fd.title, \
            fd.link, \
            fd.description \
            FROM rss_feed_source AS fs INNER JOIN \
            rss_feed_data AS fd ON fs.id = fd.rss_feed_source_id \
                                           WHERE fs.id = @id \
                                                         ORDER BY \
                                                         fd.pub_date, \
            fd.title;\
        ";

        auto sql_param_binding = create_binding("@id", std::to_string(feed_source.id), parameter_data_type::integer);

        parameter_values.push_back(sql_param_binding);
      }
      else if(!feed_source.name.empty())
      {
        sql_text =
            "SELECT \
            fs.name AS feed_name, \
            fd.pub_date, \
            fd.title, \
            fd.link, \
            fd.description \
            FROM rss_feed_source AS fs INNER JOIN \
            rss_feed_data AS fd ON fs.id = fd.rss_feed_source_id \
                                           WHERE fs.name = @feed_name \
                                                           ORDER BY \
                                                           fd.pub_date, \
            fd.title;\
        ";

        auto sql_param_binding = create_binding("@feed_name", feed_source.name, parameter_data_type::text);

        parameter_values.push_back(sql_param_binding);
      }

      std::shared_ptr<std::vector<std::map<std::string, std::string>>> query_values;
      query_values.reset(new std::vector<std::map<std::string, std::string>>);

      apply_sql(&db_connection, sql_text, parameter_values, query_values);

      if(query_values && !query_values->empty())
      {
        for(auto& row_of_data : *query_values)
        {
          const std::string feed_name = row_of_data["feed_name"];

          gautier::rss_model::unit_type_rss_item feed_item;

          make_feed_item(row_of_data, feed_item);

          tmp_rss_feed_items[feed_name].push_back(feed_item);
        }
      }
    }
  }

  rss_feed_items = tmp_rss_feed_items;

  return;
}

void 
gautier::rss_model::load_feed(const std::string feed_source_name, std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items)
{
  gautier::rss_model::unit_type_rss_source feed_source;

  feed_source.name = feed_source_name;

  load_feed(feed_source, rss_feed_items);

  return;
}

/*void 
gautier::rss_model::create_feed_items_list(const std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items, std::vector<unit_type_rss_item>& rss_items)
{
  for(const auto& named_list : rss_feed_items)
  {
    std::vector<gautier::rss_model::unit_type_rss_item> feed_list = named_list.second;

    for(gautier::rss_model::unit_type_rss_item& feed_item : feed_list)
    {
      unit_type_rss_item rss_item;

      rss_items.push_back(rss_item);
    }

    break;
  }

  return;
}*/

//Presents rss feed information to standard output.
//An optional, convenience function primarily for diagnostic/testing purposes 
//	but whose sequence can be adapted to other file based output processes.
//One of those processes can involve converting the rss feed information into 
//	HTML file format.
//HTML output is not designed into this version, but would be a quick way 
//	to put feeds into a format that can be immediately used in a web browser.
void 
gautier::rss_model::output_feeds(const std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items)
{
  const std::string heading_line = "***********************************************";

  std::stringstream ostr;

  //each feed.
  for(const auto& named_list : rss_feed_items)
  {
    const std::string& list_name = named_list.first;

    ostr << heading_line << "\n";
    ostr << "\t\t" << list_name << "\n";
    ostr << heading_line << "\n";

    std::vector<gautier::rss_model::unit_type_rss_item> feed_list = named_list.second;

    //Each value is an anonymous item that is a list of name/value pairs
    for(gautier::rss_model::unit_type_rss_item& feed_item : feed_list)
    {
      ostr << "------ details ----------------\n";

      ostr << "Title\t" << feed_item.title << "\r\n";
      ostr << "Link\t" << feed_item.link << "\r\n";
      ostr << "Description\t" << feed_item.description << "\r\n";
      ostr << "Publication Date\t" << feed_item.pubdate << "\r\n";
    }
  }

  std::cout << ostr.str();

  return;
}

//Private, module level implementation.
//Encapsulates all the logic necessary to determine which rss sources to pull fresh.
//Uses an embedded database to conduct the filtering and incorporate/extract data.

//The main purpose is to avoid unnecessary repeat calls to an rss provider.
//There are simpler and easier ways to accomplish this other than the approach chosen.
//The reason this approach was chosen, boilerplate and all, was due to the fact
//that individual rss feeds data would not be kept in separate files.
//They could, and they were for testing purposes, but the final solution is to have no 
//external parts other than network calls and a single database file. Self-contained program.
//Consolidating them in an embedded database is more useful but that comes with a complexity cost.
void 
gautier::rss_model::filter_feeds_source(const std::map<std::string, gautier::rss_model::unit_type_rss_source>& feed_sources, std::map<std::string, gautier::rss_model::unit_type_rss_source>& final_feed_sources)
{
  enable_op_sql_autolog();

  sqlite3* db_connection = nullptr;

  db_check_database_exist(&db_connection);

  if(db_connection)
  {
    std::shared_ptr<sqlite3> db_connection_guard(db_connection, db_connection_guard_finalize);

    bool tables_exist = db_check_tables_exist(&db_connection);

    if(tables_exist)
    {
      //IMPORT RSS FEED SOURCES.
      {
        db_transact_begin(&db_connection);

        std::string sql_text = "INSERT INTO rss_feed_source(name, url) VALUES (trim(@name), trim(@url));";

        for(const auto& feed_source : feed_sources)
        {
          std::vector<std::tuple<std::string, std::string, parameter_data_type>> parameter_values =
          {
            create_binding("@name", feed_source.second.name, parameter_data_type::text),
            create_binding("@url", feed_source.second.url, parameter_data_type::text)
          };

          apply_sql(&db_connection, sql_text, parameter_values, nullptr);
        }

        db_transact_end(&db_connection);
      }

      //COLLECT RSS FEED NAME CHANGES.
      {
        char* error_message = 0;
        auto sql_query_exec_result = SQLITE_BUSY;

        std::shared_ptr<std::vector<std::map<std::string, std::string>>> query_values;
        query_values.reset(new std::vector<std::map<std::string, std::string>>);

        {
          std::string sql_text =
              "SELECT \
              dest.id AS dest_id, \
              dest.name AS dest_name, \
              src.id AS src_id, \
              src.name AS src_name \
              FROM (SELECT id, name, url FROM rss_feed_source WHERE type_code = 0) AS dest INNER JOIN \
              (SELECT id, name, url FROM rss_feed_source WHERE type_code = 1) AS src ON dest.url = src.url \
                                                                                                   COLLATE NOCASE; \
          ";

          sql_query_exec_result = sqlite3_exec(db_connection, sql_text.data(), translate_sql_result, query_values.get(), &error_message);
        }

        //RECONCILE OLD NAMES WITH NEW.
        if(sql_query_exec_result == SQLITE_OK)
        {
          if(query_values)
          {
            db_transact_begin(&db_connection);

            //APPLY NEW NAMES TO OLD.
            for(const auto& row_of_data : *query_values)
            {
              //If the input names changed,
              //but the url stayed the same, update the names to match.
              std::string sql_text = "UPDATE rss_feed_source SET name = @name WHERE id = @id;";

              std::vector<std::tuple<std::string, std::string, parameter_data_type>> parameter_values =
              {
                create_binding("@name", get_first_db_column_value(row_of_data, "src_name"), parameter_data_type::text),
                create_binding("@id", get_first_db_column_value(row_of_data, "dest_id"), parameter_data_type::integer)
              };

              apply_sql(&db_connection, sql_text, parameter_values, nullptr);
            }

            //REMOVE DUPLICATE RSS FEED SOURCES.
            //Once the names are synched,
            //remove the source update data.
            for(const auto& row_of_data : *query_values)
            {
              std::string sql_text = "DELETE FROM rss_feed_source WHERE id = @id;";

              std::vector<std::tuple<std::string, std::string, parameter_data_type>> parameter_values =
              {
                create_binding("@id", get_first_db_column_value(row_of_data, "src_id"), parameter_data_type::integer)
              };

              apply_sql(&db_connection, sql_text, parameter_values, nullptr);
            }

            db_transact_end(&db_connection);
          }
        }
        else
        {
          output_op_sql_error_message(&error_message, __LINE__);
        }
      }

      //ACCEPT REMAINING RSS FEED SOURCES.
      {
        db_transact_begin(&db_connection);

        //Any remaining entries will be accepted.
        std::string sql_text = "UPDATE rss_feed_source set type_code = 0 WHERE type_code = 1";

        apply_sql(&db_connection, sql_text, _empty_param_set, nullptr);

        db_transact_end(&db_connection);
      }

      /***
        MAIN SQL QUERY.
        Data output drives the main program.
      ***

      Websites and networks hosting RSS data may have a policy
        limiting how often those networks can be queried for RSS information
        from a given computer.
      The query below is written under the default view that this policy is in effect.
      A minimum time frame of 1 hour has been chosen as the length of the window
        before a query can once again be issued for a network source of RSS data.

      This time window could be parameterized to allow an extension of the window.
        The ability to vary the time window is not encoded in this version
        since that is was not a desired capability. It can be trivially
        introduced, but if it is, it is recommended that a small validation check
        be introduced to enforce the suggested minimum time window of 1 hour.

      The query uses the type_code field to represent the status of the data.
      A type_code of 0 means the rss feed is unchanged in status from the last time
        a query was applied to it. That means the saved feeds data should be used.

      A type_code of 1 means the rss feed name changed. The application should never
        see a type_code of one as that is an intermediate status. SQL is used to
        managed the transformation from type_code 1 to 0.

      A type_code of 3 means the rss feed should be clear to query for new data.
        The application may use this as a top-level hint about which feeds
        may have changed recently. This can be a quick hint that saves traversing
        the full data set.
      There is no need for the application to use this to decide when to issue queries.
      All the application has to do is execute the public functions for this module.
      The logic within this module determines the actual timing of when to pull new RSS feed data.

      No status or in-progress information is provided in this version. If that is ever
        desired, perhaps for a user interface to animate status, then a callback
        would be the logical choice to provide this level of communication.

      Based on the above description, the SQL is defined as follows:
        An SQL CASE statement evaluates the entry date field.
        The first case branch deals with rows imported for the first time.
      Scenario #1
        Those first-time rows are set to indicate that RSS feed data should be
        gathered during the next pass query for RSS feed data from networks.
      Scenario #2
        The second case branch is the time window spoken about earlier and is
        what determines when RSS feed data is actually refreshed.
      Scenario #3
        The third branch of the CASE statement returns the type_code as is which
        should normally indicate no action is required for a given RSS feed source.

      The overall program's activity regarding RSS data collection and organization
        is primarily affected by the shape of the data determined by the
        SQL engine when evaluating this query on the data stored.
      */

      std::string sql_text =
          "SELECT \
          \
          id,\
          CASE \
          WHEN (datetime(entry_date, '+1 minute')) > (datetime('now', 'localtime')) \
          THEN 3 \
          WHEN (datetime(entry_date, '+1 hour')) < (datetime('now', 'localtime')) \
          THEN 3 \
          ELSE type_code \
          END AS type_code,\
          entry_date,\
          name,\
          url\
          FROM rss_feed_source;\
      ";

      std::shared_ptr<std::vector<std::map<std::string, std::string>>> query_values;
      query_values.reset(new std::vector<std::map<std::string, std::string>>);

      char* error_message = 0;

      const auto sql_query_exec_result = sqlite3_exec(db_connection, sql_text.data(), translate_sql_result, query_values.get(), &error_message);

      if(sql_query_exec_result == SQLITE_OK)
      {
        if(query_values && !query_values->empty())
        {
          for(auto& row_of_data : *query_values)
          {
            gautier::rss_model::unit_type_rss_source rss_source;

            rss_source.id = std::stoi(row_of_data["id"]);
            rss_source.type_code = std::stoi(row_of_data["type_code"]);
            rss_source.name = row_of_data["name"];
            rss_source.url = row_of_data["url"];

            final_feed_sources[rss_source.name] = rss_source;
          }
        }
      }
      else
      {
        output_op_sql_error_message(&error_message, __LINE__);
      }

    }//end of table scope
  }

  return;
}

//THE TRUE TRIGGER FOR RSS DOWNLOAD.
//The idea is to update the main entry date whenever a feed is accessed over the network.
void 
gautier::rss_model::save_feeds(const std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items)
{
  sqlite3* db_connection = nullptr;

  if(!rss_feed_items.empty())
  {
    db_check_database_exist(&db_connection);
  }

  if(db_connection)
  {
    std::shared_ptr<sqlite3> db_connection_guard(db_connection, db_connection_guard_finalize);

    enable_op_sql_trace(&db_connection);

    db_transact_begin(&db_connection);

    for(const auto& rss_feed_item : rss_feed_items)
    {
      int rss_feed_source_id = 0;

      const std::string rss_feed_name = rss_feed_item.first;
      std::vector<gautier::rss_model::unit_type_rss_item> feed_items = rss_feed_item.second;

      //GET RSS FEED DESCRIPTION RECORD.
      {
        std::string sql_text =
            "SELECT \
            id \
            FROM rss_feed_source \
            WHERE name = @name; \
        ";

        std::vector<std::tuple<std::string, std::string, parameter_data_type>> parameter_values =
        {
          create_binding("@name", rss_feed_name, parameter_data_type::text)
        };

        std::shared_ptr<std::vector<std::map<std::string, std::string>>> query_values;
        query_values.reset(new std::vector<std::map<std::string, std::string>>);

        apply_sql(&db_connection, sql_text, parameter_values, query_values);

        if(query_values)
        {
          for(auto& row_of_data : *query_values)
          {
            std::string id_text_value = row_of_data["id"];

            if(!id_text_value.empty())
            {
              rss_feed_source_id = std::stoi(id_text_value);
            }

            break;
          }
        }

        //ABORTS THE ENTIRE OPERATION for this feed.
        //Without a source_id, there is no linkage that can be made.
        if(rss_feed_source_id == 0)
        {
          std::cout << __FILE__ " " << __func__ << " " << "(" << __LINE__ << ") ";
          std::cout
              << "not adding feed "
              << rss_feed_name << ", see:"
              << __func__
              << "\n";

          break;
        }
      }

      for(auto& feed_item : feed_items)
      {
        //IMPORT RSS FEED DATA.
        {
          std::string sql_text =
              "INSERT INTO rss_feed_data_staging \
              (\
                rss_feed_source_id,\
                pub_date,\
                title,\
                link,\
                description\
                )\
              VALUES \
              (\
                @rss_feed_source_id,\
                @pub_date,\
                trim(@title),\
                trim(@link),\
                trim(@description)\
                )\
              ;";

          std::vector<std::tuple<std::string, std::string, parameter_data_type>> parameter_values =
          {
            create_binding("@rss_feed_source_id", std::to_string(rss_feed_source_id), parameter_data_type::integer),
            create_binding("@pub_date", feed_item.pubdate, parameter_data_type::text),
            create_binding("@title", feed_item.title, parameter_data_type::text),
            create_binding("@link", feed_item.link, parameter_data_type::text),
            create_binding("@description", feed_item.description, parameter_data_type::text)
          };

          apply_sql(&db_connection, sql_text, parameter_values, nullptr);
        }
      }
    }

    db_transact_end(&db_connection);

    //Transfers eligible feeds data entries from staging to active.
    //The staging data remains in place for diagnostic purposes.
    //Older entries will be purged from both tables.
    {
      db_transact_begin(&db_connection);

      std::string
          sql_text =
          "INSERT INTO rss_feed_data ( \
          rss_feed_source_id, \
          pub_date, \
          title, \
          link, \
          description \
          ) \
          SELECT \
          rss_feed_source_id, \
          pub_date, \
          title, \
          link, \
          description \
          FROM rss_feed_data_staging \
          WHERE link NOT IN ( \
            SELECT \
            link \
            FROM rss_feed_data \
            GROUP BY link \
            ) GROUP BY \
          rss_feed_source_id, \
          pub_date, \
          title, \
          link, \
          description \
          ORDER BY \
          rss_feed_source_id, \
          pub_date DESC, \
          title \
          ; \
      DELETE \
          FROM rss_feed_data_staging \
          WHERE (datetime(entry_date, '+8 hour')) < (datetime('now', 'localtime'));\
      DELETE \
          FROM rss_feed_data \
          WHERE (datetime(entry_date, '+1 month')) < (datetime('now', 'localtime'));\
      ";

      apply_sql(&db_connection, sql_text, _empty_param_set, nullptr);

      db_transact_end(&db_connection);
    }
  }

  return;
}

void 
gautier::rss_model::make_feed_item(std::map<std::string, std::string>& row_of_data, gautier::rss_model::unit_type_rss_item& feed_item)
{
  feed_item.title = row_of_data["title"];
  feed_item.link = row_of_data["link"];
  feed_item.description = row_of_data["description"];
  feed_item.pubdate = row_of_data["pub_date"];

  return;
}

//Retrieves rss data at a given url, decodes the XML into a data structure named, std::map<std::string, std::vector<std::map<std::string, std::string>>>.
//Retrieval logic is done by the xml library which will pull from a file location or web address.
//After retrieval, xml represented as various libxml objects.
//The linked list is traversed in a compact sequenct that converts entries in a std::map<std::string, std::vector<std::map<std::string, std::string>>> data structure.
void 
gautier::rss_model::collect_feed_items_from_rss(const std::map<std::string, gautier::rss_model::unit_type_rss_source>& feed_sources, std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items)
{
  for(auto& feed_source : feed_sources)
  {
    if(feed_source.second.type_code == 3)//collect from feeds past expire date.
    {
      //see libxml2 tree1.c example file for the general structure used.
      LIBXML_TEST_VERSION

          auto xml_parse_options = (XML_PARSE_RECOVER | XML_PARSE_NOERROR | XML_PARSE_NOWARNING | XML_PARSE_NOBLANKS | XML_PARSE_NOCDATA);

      xmlDoc *doc = xmlReadFile(feed_source.second.url.data(), nullptr, xml_parse_options);

      if(doc)
      {
        xmlNode *root_element = xmlDocGetRootElement(doc);

        if(root_element)
        {
          std::vector<gautier::rss_model::unit_type_rss_item> rss_feed_values;

          rss_feed_values.reserve(_list_reserve_size);

          rss_feed_items[feed_source.first] = rss_feed_values;

          collect_feed_items(root_element, rss_feed_items[feed_source.first]);
        }

        xmlFreeDoc(doc);
        xmlCleanupParser();
      }
    }
  }

  return;
}

//See libxml2 tree1.c example file for the general structure used. 9/24/2015
void 
gautier::rss_model::collect_feed_items(xmlNode* xml_element, std::vector<gautier::rss_model::unit_type_rss_item>& feed_items)
{
  for(xmlNode* current_node = xml_element; (current_node != nullptr); current_node = current_node->next)
  {
    if(current_node->type == XML_ELEMENT_NODE)
    {
      const std::string current_local_name = get_string_from_xmlchar(current_node->name, switch_letter_case);

      if(current_local_name == _element_name_item)
      {
        feed_items.push_back(gautier::rss_model::unit_type_rss_item());
      }
      else if(is_an_approved_rss_data_name(current_local_name))
      {
        const std::string parent_local_name = get_string_from_xmlchar(current_node->parent->name, nullptr);

        if(parent_local_name == _element_name_item && !feed_items.empty())
        {
          gautier::rss_model::unit_type_rss_item& feed_item = feed_items.back();

          std::string node_data;
          {
            xmlChar* node_value = xmlNodeGetContent(current_node);

            node_data = get_string_from_xmlchar(node_value, nullptr);

            xmlFree(node_value);
          }

          if(current_local_name == "title")
          {
            feed_item.title = node_data;
          }
          else if(current_local_name == "link")
          {
            feed_item.link = node_data;
          }
          else if(current_local_name == "description")
          {
            feed_item.description = node_data;
          }
          else if(current_local_name == "pub_date")
          {
            feed_item.pubdate = node_data;
          }
        }
      }
    }

    collect_feed_items(current_node->children, feed_items);
  }

  return;
}

std::string 
gautier::rss_model::get_string_from_xmlchar(const xmlChar* xstring_in, decltype(switch_letter_case) transform_func)
{
  std::string value;

  if(xstring_in)
  {
    std::stringstream ostr;

    ostr << xstring_in;

    value = ostr.str();
  }

  if(transform_func && !value.empty())
  {
    std::transform(value.begin(), value.end(), value.begin(), transform_func);
  }

  return value;
}

//Defines interest in specific element names from the input XML data.
//This confirms that an element name exists in the list of approved
//	element names. Other code uses this to determine if data 
//	from an element should be retrieved.
bool 
gautier::rss_model::is_an_approved_rss_data_name(const std::string& element_name)
{
  bool match_found = false;

  std::string data_name = element_name;

  if(!data_name.empty())
  {
    std::transform(data_name.begin(), data_name.end(), data_name.begin(), switch_letter_case);

    auto found = std::find(_element_names.cbegin(), _element_names.cend(), data_name);

    match_found = (found != _element_names.cend());
  }

  return match_found;
}

//The goal of the following operations is to produce a data structure of type std::map<std::string, std::vector<std::map<std::string, std::string>>>.
//Manage access to a database that contains the data used to form the data structure.

//SQL: Database infrastructure/tables.

//The following functions under this section deals with supporting database structures for the rss engine.

//Governs the deallocation of an sqlite3 pointer through a pointer resource handle.
static void 
db_connection_guard_finalize(sqlite3* obj)
{
  if(obj)
  {
    sqlite3_close(obj);
  }

  return;
}

//Make a database file if one does not exist.
//Not currently a halting error if this fails. 
//Rather, the process fails silently if a database cannot be made available.
bool 
gautier::rss_model::db_check_database_exist(sqlite3** db_connection)
{
  bool success = false;

  auto sqlite_options = (SQLITE_OPEN_PRIVATECACHE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

  const auto open_result = sqlite3_open_v2(_rss_database_name.data(), db_connection, sqlite_options, nullptr);

  if(open_result == SQLITE_OK && db_connection)
  {
    success = true;
  }
  else
  {
        std::cout << __FILE__ " " << __func__ << " " << "(" << __LINE__ << ") ";
        std::cout
        << "unable to open database.\n";
  }

  return success;
}

//Examines the database for the existence of tables.
//Uses an aggregate function to determine if a table exists.
//If the table does not exist, dispatches a call to db_create_table, to define the table in the database.
bool 
gautier::rss_model::db_check_tables_exist(sqlite3** db_connection)
{
  bool success = false;

  if(db_connection)
  {
    const auto expected = _table_names.size();

    decltype(_table_names)::size_type actual = 0;

    for(const std::string& table_name : _table_names)
    {
      std::string
          sql_text =
          "SELECT \
          COUNT(*) AS row_count \
          FROM sqlite_master \
          WHERE type='table' \
                     AND name = @table_name; \
      ";

      std::vector<std::tuple<std::string, std::string, parameter_data_type>> parameter_values =
      {
        create_binding("@table_name", table_name, parameter_data_type::text)
      };

      std::shared_ptr<std::vector<std::map<std::string, std::string>>> query_values;
      query_values.reset(new std::vector<std::map<std::string, std::string>>);

      apply_sql(db_connection, sql_text, parameter_values, query_values);

      if(query_values)
      {
        const auto row_count = std::stoul(get_first_db_column_value(query_values->front(), "row_count"));

        const bool exists = (row_count > 0);

        if(exists)
        {
          actual++;//note the existence of the table.
        }
        else//not exists
        {
          const bool table_created = db_create_table(db_connection, table_name);

          if(table_created)
          {
            actual++;//note its existence.
          }
        }
      }
    }

    success = (actual == expected);
  }

  return success;
}

//Defines tables in the relational database used by the rss engine.
bool 
gautier::rss_model::db_create_table(sqlite3** db_connection, const std::string& table_name)
{
  bool success = false;

  if(db_connection)
  {
    std::string db_create_table_statement_text = "";

    if(table_name == "rss_feed_source")
    {
      db_create_table_statement_text =
          "CREATE TABLE " + table_name + "\
          (\
            id INTEGER PRIMARY KEY ASC,\
            type_code INTEGER DEFAULT 1,\
            entry_date TEXT DEFAULT (datetime(CURRENT_TIMESTAMP, 'localtime')),\
            name TEXT NOT NULL COLLATE NOCASE,\
            url TEXT NOT NULL COLLATE NOCASE\
            );\
      ";
    }
    else if(table_name == "rss_feed_data" || table_name == "rss_feed_data_staging")
    {
      db_create_table_statement_text =
          "CREATE TABLE " + table_name + "\
          (\
            id INTEGER PRIMARY KEY ASC,\
            rss_feed_source_id INTEGER NOT NULL,\
            entry_date TEXT DEFAULT (datetime(CURRENT_TIMESTAMP, 'localtime')),\
            pub_date TEXT NOT NULL,\
            title TEXT NOT NULL COLLATE NOCASE,\
            link TEXT NOT NULL COLLATE NOCASE,\
            description TEXT NOT NULL COLLATE NOCASE\
            );\
      ";
    }

    if(!db_create_table_statement_text.empty())
    {
      char* error_message = 0;

      const auto sqlite_result = sqlite3_exec(*db_connection, db_create_table_statement_text.data(), nullptr, nullptr, &error_message);

      if(sqlite_result == SQLITE_OK)
      {
        success = true;
      }
      else
      {
        output_op_sql_error_message(&error_message, __LINE__);
      }
    }
  }

  return success;
}

bool 
gautier::rss_model::db_transact_begin(sqlite3** db_connection)
{
  std::string sql_text = "BEGIN IMMEDIATE TRANSACTION";

  auto sqlite_result = apply_sql(db_connection, sql_text, _empty_param_set, nullptr);

  return sqlite_result.first;
}

bool 
gautier::rss_model::db_transact_end(sqlite3** db_connection)
{
  std::string sql_text = "COMMIT TRANSACTION";

  auto sqlite_result = apply_sql(db_connection, sql_text, _empty_param_set, nullptr);

  return sqlite_result.first;
}

//SQL: Queries

//Links a named parameter to data value for use in a parameterized sql statement.
std::tuple<std::string, std::string, gautier::rss_model::parameter_data_type> 
gautier::rss_model::create_binding(const std::string name, const std::string value, const gautier::rss_model::parameter_data_type parameter_type)
{
  return std::tuple<std::string, std::string, parameter_data_type>(name, value, parameter_type);
}

//Execute an sql statement.
//At the same level as the sqlite3_exec function.
//The sqlite3_exec function is the preferred way for any readonly queries that do not take parameters.
//The primary use of this function is to introduce parameters to an sql statement.
//Otherwise, it produces the same output as the sqlite3_exec callback for this module, translate_sql_result.
//A noteable difference with this function versus translate_sql_result is that this function will 
//	immediately output all error messages to std out rather than return an error data structure.
//As a result, the return SQLITE result code/error code is primarily for control caller control flow.
std::pair<bool, int> 
gautier::rss_model::apply_sql(sqlite3** db_connection, std::string& sql_text, std::vector<std::tuple<std::string, std::string, parameter_data_type>>& parameter_binding_infos, std::shared_ptr<std::vector<std::map<std::string, std::string>>> query_values)
{
  bool success = false;
  int row_count = -1;

  sqlite3_stmt* sql_stmt = nullptr;

  if(!db_connection || sql_text.empty())
  {
    return std::pair<bool, int>(success, row_count);
  }

  auto sqlite_prepare_result = SQLITE_BUSY;
  {
    const char* sql_char_ptr = sql_text.data();
    int sql_char_ptr_sz = -1;

    sqlite_prepare_result = sqlite3_prepare_v2(*db_connection, sql_char_ptr, sql_char_ptr_sz, &sql_stmt, nullptr);
  }

  if(sqlite_prepare_result == SQLITE_OK)
  {
    int params_count = 0;

    //Binding the input values to the input sql statement.
    for(auto& bind_info : parameter_binding_infos)
    {
      params_count++;

      int param_n = params_count;

      const std::string parameter_name = std::get<0>(bind_info);
      const std::string parameter_text = std::get<1>(bind_info);
      const parameter_data_type param_t = std::get<2>(bind_info);

      auto sqlite_result = 0;

      if(param_t == parameter_data_type::text)
      {
        const char* param_char_ptr = parameter_text.data();
        const int param_char_ptr_sz = -1;

        sqlite3_bind_text(sql_stmt, param_n, param_char_ptr, param_char_ptr_sz, SQLITE_TRANSIENT);
      }
      else if(param_t == parameter_data_type::integer)
      {
        const int param_value = std::stoi(parameter_text);

        sqlite3_bind_int(sql_stmt, param_n, param_value);
      }
      else
      {
        if(param_t != parameter_data_type::none)
        {
          std::cout << __FILE__ " " << __func__ << " " << "(" << __LINE__ << ") ";
          std::cout
              << "parameter " << parameter_name
              << " data type not mapped.\r\n";
        }
      }

      if(sqlite_result != SQLITE_OK)
      {
        output_op_sql_error_message(db_connection, __LINE__);

        break;
      }
    }

    if(static_cast<type_list_size>(params_count) != parameter_binding_infos.size())
    {
      success = false;

      std::cout << __FILE__ " " << __func__ << " " << "(" << __LINE__ << ") ";
      std::cout
          << "could not bind params to: " << sql_text
          << "\r\n";

      std::cout
          << "total parameters : " << parameter_binding_infos.size()
          << "\r\n";

      for(auto& diag_bind_info : parameter_binding_infos)
      {
        std::cout
            << std::get<0>(diag_bind_info) << " | "
            << std::get<1>(diag_bind_info) << " | "
            << std::to_string(static_cast<type_list_size>(std::get<2>(diag_bind_info)))
            << "\n";
      }
    }
    else
    {
      //Run the SQL.
      auto sqlite_result = SQLITE_BUSY;

      if(!sqlite3_stmt_readonly(sql_stmt))
      {
        sqlite_result = sqlite3_step(sql_stmt);
      }
      else
      {
        do
        {
          sqlite_result = sqlite3_step(sql_stmt);

          if(sqlite_result == SQLITE_ROW)
          {
            success = translate_sql_result(query_values, sql_stmt);
          }
        }while(sqlite_result != SQLITE_DONE);
      }

      if(sqlite_result == SQLITE_DONE)
      {
        success = true;

        row_count = sqlite3_changes(*db_connection);
      }
      else
      {
        success = false;//undoing any success up to this point.

        output_op_sql_error_message(db_connection, __LINE__);
      }
    }
  }
  else
  {
    output_op_sql_error_message(db_connection, __LINE__);
  }

  //diagnostic
  if(_issued_sql_output_enabled)
  {
    auto sql_statement_text = sqlite3_sql(sql_stmt);

    std::cout << "\t\t ran "
              << sql_statement_text;

    if(success)
    {
      std::cout
          << ". the result was good. " ;

      if(_rows_affected_output_enabled)
      {
        std::cout << row_count << " rows affected. ";
      }
    }
    else
    {
      std::cout << __FILE__ " " << __func__ << " " << "(" << __LINE__ << ") ";
      std::cout << ". failed. ";
    }

    std::cout << "\r\n";
  }
  else
  {
    if(_rows_affected_output_enabled)
    {
      std::cout
          << row_count
          << " rows affected.\n";
    }
  }

  if(sql_stmt)
  {
    sqlite3_reset(sql_stmt);//clean it up so finalize can succeed.

    const auto sqlite_result = sqlite3_finalize(sql_stmt);

    if(sqlite_result != SQLITE_OK)
    {
      success = false;

      output_op_sql_error_message(db_connection, __LINE__);
    }
  }

  return std::pair<bool, int>(success, row_count);
}

//Converts the columns in an sqlite3_stmt structure to query_value rows;
static bool 
translate_sql_result(std::shared_ptr<std::vector<std::map<std::string, std::string>>> query_values, sqlite3_stmt* sql_stmt)
{
  bool success = false;

  if(query_values)
  {
    std::map<std::string, std::string> row_of_data;

    type_list_size total_columns = 0L;
    {
      using n_unit = int;

      const n_unit max_columns = sqlite3_column_count(sql_stmt);

      for(n_unit col_n = 0; col_n < max_columns; col_n++)
      {
        const std::string column_name = sqlite3_column_name(sql_stmt, col_n);

        std::stringstream ostr;

        ostr << sqlite3_column_text(sql_stmt, col_n);

        row_of_data[column_name] = ostr.str();
      }

      total_columns = static_cast<type_list_size>(max_columns);
    }

    success = (total_columns == row_of_data.size());

    if(success)
    {
      query_values->push_back(row_of_data);
    }
    else
    {
      std::cout << "requested "
                << total_columns << " columns "
                << "processed only " << row_of_data.size()
                << ".\r\n";
    }
  }

  return success;
}

//Callback function for sqlite3_exec.
//Defined to build a data structure, std::vector<std::map<std::string, std::string>> that represents a tabular result set.
//This result set can be traversed to calling process that issued the sql statement used to generate it.
static int 
translate_sql_result(void* user_defined_data, int column_count, char** column_values, char** column_names)
{
  std::vector<std::map<std::string, std::string>>* query_values = nullptr;

  if(user_defined_data)
  {
    query_values = static_cast<std::vector<std::map<std::string, std::string>>*>(user_defined_data);
  }

  std::map<std::string, std::string> row_of_data;//This is the actual row of data.

  using n_unit = decltype(column_count);

  //every column for the row should be accessible from here.
  //transfer each column to a corresponding column in the row_of_data variable.
  for(n_unit column_index = 0; column_index < column_count; column_index++)
  {
    const char* column_name_chars = column_names[column_index];
    const char* column_value_chars = column_values[column_index];

    if(query_values)
    {
      row_of_data[column_name_chars] = column_value_chars;
    }
    else if(_query_values_translator_output_enabled)
    {
      std::cout
          << "column name:" << column_name_chars
          << "| column value:" << column_value_chars
          << "\n";
    }
  }

  if(query_values)
  {
    query_values->push_back(row_of_data);
  }

  return 0;
}

//Goes through the query value lines. The value of the 
//	very first column key name that matches the input is returned.
std::string 
gautier::rss_model::get_first_db_column_value(const std::map<std::string, std::string>& row_of_data, const std::string& col_name)
{
  std::string col_value = "";

  for(auto& column : row_of_data)
  {
    if(column.first == col_name)
    {
      col_value = column.second;

      break;
    }
  }

  return col_value;
}

//SQL: Diagnostics

//Diagnostics support for SQLite3.
//----------------------------------------------------------
//The following functions are used for debugging, profiling use of SQLite3.
void 
gautier::rss_model::output_data_rows(const std::shared_ptr<std::vector<std::map<std::string, std::string>>> query_values)
{
  if(query_values)
  {
    for(const auto& row : *query_values)
    {
      for(const auto& column : row)
      {
        std::cout << "column name:" << column.first << "| column value:" << column.second << "\n";
      }
    }
  }

  return;
}

static void 
enable_op_sql_trace(sqlite3** db_connection)
{
  if(_sql_trace_enabled && !_sql_trace_active)
  {
    sqlite3_trace(*db_connection, trace_sql_op, nullptr);

    _sql_trace_active = true;
  }

  return;
}

static void 
trace_sql_op(void* in1, const char* in2)
{
  if(_sql_trace_active)
  {
    if(in1)
    {
      std::cout << " ";
    }

    std::cout << in2 << "\n";
  }

  return;
}

static void 
enable_op_sql_autolog()
{
  if(_sql_trace_enabled)
  {
    sqlite3_config(SQLITE_CONFIG_LOG, log_sql_op_event, nullptr);
  }

  return;
}

static void 
log_sql_op_event(void *pArg, int iErrCode, const char *zMsg)
{
  if(_sql_trace_enabled && _sql_trace_active)
  {
    if(!pArg)
    {
        std::cout << __FILE__ " " << __func__ << " " << "(" << __LINE__ << ") argument is invalid.";
    }

    std::cout << __FILE__ " " << __func__ << " " << "(" << __LINE__ << ") ";
    std::cout
        << iErrCode << " "
        << zMsg;
  }

  return;
}

static void
output_op_sql_error_message(sqlite3** db_connection, const int& line_number)
{
        std::cout << __FILE__ " " << __func__ << " " << "(" << __LINE__ << ") ";
        std::cout
                << sqlite3_errmsg(*db_connection) << " "
                << line_number
                << "\r\n";

  return;
}

static void
output_op_sql_error_message(char** error_message, const int& line_number)
{
        std::cout << __FILE__ " " << __func__ << " " << "(" << __LINE__ << ") ";
        std::cout
                << *error_message << " "
                << line_number
                << "\r\n";

        sqlite3_free(*error_message);

        *error_message = nullptr;

  return;
}

//Support functions.
static int
switch_letter_case (const char& in_char)
{
  return std::tolower(in_char);
}

//Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Software distributed under the License is distributed on an "AS IS" BASIS, NO WARRANTIES OR CONDITIONS OF ANY KIND, explicit or implicit. See the License for details on permissions and limitations.

