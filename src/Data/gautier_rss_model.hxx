#ifndef __gautier_rss_model__
#define __gautier_rss_model__

#include <string>
#include <map>
#include <vector>

#include <sqlite3.h>

#include <cstdio>
#include <libxml/parser.h>
#include <libxml/tree.h>

namespace gautier
{
	class rss_model
	{
	        public:
	        rss_model();
	        
		struct unit_type_rss_source
		{
			int 
				id{0},
				type_code{0}
			;

			std::string 
				name{""},
				url{""}
			;
		};

		struct unit_type_rss_item
		{
			int 
				id{0}
			;

			std::string 
				title{},
				link{},
				description{},
				pubdate{}
			;
		};

		//Should always call this at least once before any other function in this module.
		void 
		load_feeds_source_list(const std::string& feeds_list_file_name, std::map<std::string, unit_type_rss_source>& feed_sources);

		//Reloads source list from cache with updated expiration indicator.
		void 
		load_feeds_source_list(std::map<std::string, unit_type_rss_source>& feed_sources);

		//Collects and saves feeds.
		//Gathered feed items can be retrieved more selectively by the application.
		//*Recommended way to gather feed items.
		void 
		collect_feeds(const std::map<std::string, unit_type_rss_source>& feed_sources);

		//Collects and saves feeds and returns the list of all feed items collected.
		//Best used for caching all feed items for all feed sources.
		//Simplest way to execute the rss feed engine but accumulates all data into memory.
		//Most useful for running full tests of the overall feed gather and output process.
		void 
		collect_feeds(const std::map<std::string, unit_type_rss_source>& feed_sources, std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items);

		//Returns all rss feed items previously collected.
		//Useful for caching all feeds items previously collected.
		void 
		load_feeds(std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items);

		//Returns all rss feed items previously collected for an rss feed source.
		//*Recommended way to access feed items after collecting them.
		void 
		load_feed(const unit_type_rss_source& feed_source, std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items);

		//Returns all rss feed items previously collected for an rss feed source.
		//Provides a convenient way to access feed items after collecting them.
		//Matches feeds by name of the feed source.
		void 
		load_feed(const std::string feed_source_name, std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items);

		//void 
		//create_feed_items_list(const std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items, std::vector<unit_type_rss_item>& rss_items);

		//output to std out.
		//terminal output.
		//possible, future output to html file.
		void 
		output_feeds(const std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items);
		
		private:

                //Module level types and type aliases.
                enum parameter_data_type
                {
                  none,
                  text,
                  integer
                };

                //Implementation, module level variables.

                char 
                _comment_marker,
                _feed_config_line_sep;

                int 
                _list_reserve_size;

                std::string 
                _element_name_item,
                _rss_database_name;

                std::vector<std::string> 
                _element_names,
                _table_names;

                std::vector<std::tuple<std::string, std::string, parameter_data_type>> 
                _empty_param_set;

                /*Implementation, general support functions.*/
                //int switch_letter_case (const char& in_char);

                /*Implementation, top-level logic
                  Largely SQL API dependent.
                */
                void filter_feeds_source(const std::map<std::string, gautier::rss_model::unit_type_rss_source>& feed_sources, std::map<std::string, gautier::rss_model::unit_type_rss_source>& final_feed_sources);
                void save_feeds(const std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items);
                void make_feed_item(std::map<std::string, std::string>& row_of_data, gautier::rss_model::unit_type_rss_item& feed_item);

                /*
                  Implementation, supporting logic.
                  XML API dependent
                  Takes a source of data, defined in the XML format, using the RSS 1.0 schema
                          and converts it to various application defined data structures.
                  These output data structures drive the entire rss engine.
                          std::map<std::string, std::vector<std::map<std::string, std::string>>> and std::vector<std::map<std::string, std::string>> are the main data structures.
                */
                void collect_feed_items_from_rss(const std::map<std::string, gautier::rss_model::unit_type_rss_source>& feed_sources, std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>>& rss_feed_items);
                void collect_feed_items(xmlNode* xml_element, std::vector<gautier::rss_model::unit_type_rss_item>& feed_items);
                std::string get_string_from_xmlchar(const xmlChar* xstring_in, int (*)(const char&));
                bool is_an_approved_rss_data_name(const std::string& element_name);

                //SQL: Database infrastructure/tables.
                //void db_connection_guard_finalize(sqlite3* obj);
                bool db_check_database_exist(sqlite3** db_connection);
                bool db_check_tables_exist(sqlite3** db_connection);
                bool db_create_table(sqlite3** db_connection, const std::string& table_name);
                bool db_transact_begin(sqlite3** db_connection);
                bool db_transact_end(sqlite3** db_connection);

                //SQL: Transformation
                //int translate_sql_result(void* user_defined_data, int column_count, char** column_values, char** column_names);
                //bool translate_sql_result(std::shared_ptr<std::vector<std::map<std::string, std::string>>> query_values, sqlite3_stmt* sql_stmt);
                std::pair<bool, int> apply_sql(sqlite3** db_connection, std::string& sql_text, std::vector<std::tuple<std::string, std::string, parameter_data_type>>& parameter_binding_infos, std::shared_ptr<std::vector<std::map<std::string, std::string>>> query_values);
                std::string get_first_db_column_value(const std::map<std::string, std::string>& row_of_data, const std::string& col_name);
                std::tuple<std::string, std::string, parameter_data_type> create_binding(const std::string name, const std::string value, const parameter_data_type parameter_type);

                //SQL: Diagnostics
                void output_data_rows(const std::shared_ptr<std::vector<std::map<std::string, std::string>>> query_values);

                //void enable_op_sql_trace(sqlite3** db_connection);
                //void trace_sql_op(void*, const char*);

                //void enable_op_sql_autolog();
                //void log_sql_op_event(void *pArg, int iErrCode, const char *zMsg);

                //void output_op_sql_error_message(char** error_message, const int& line_number);
                //void output_op_sql_error_message(sqlite3** db_connection, const int& line_number);

	};
}
#endif
//Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Software distributed under the License is distributed on an "AS IS" BASIS, NO WARRANTIES OR CONDITIONS OF ANY KIND, explicit or implicit. See the License for details on permissions and limitations.

