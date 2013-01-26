/////////////////////////////////////////////////////////////////////////////
//    License (GPLv2+):
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or (at
//    your option) any later version.
//    
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
/////////////////////////////////////////////////////////////////////////////
/** 
 * @file  FilterCommentsManager.h
 *
 * @brief FilterCommentsManager class declaration.
 */

#ifndef _FILTERCOMMENTSMANAGER_H_
#define _FILTERCOMMENTSMANAGER_H_

enum OP_TYPE;
struct file_data;

//IngnoreComment logic developed by David Maisonave AKA (Axter)
/**
@struct FilterCommentsSet
@brief FilterCommentsSet holds search strings used to find comments in compared files.
		This data is used to find blocks that can be ignored when comparing to files.
@note
		The ignore-comment logic can only use ANSI strings, because the search buffer is
		char* type.
		Therefore, the data members should not be replaced with String type, and should
		remain std::string, or other non-unicode type string.
*/

struct FilterCommentsSet
{
	stl::string StartMarker;
	stl::string EndMarker;
	stl::string InlineMarker;
	static const char quote_flag_ignore = '\x7F';
	static const char *FindCommentMarker(const char *target, const stl::string &marker, char quote_flag = '\0');
	OP_TYPE PostFilter(int StartPos, int QtyLinesInBlock, const file_data *inf) const;
};

/**
@class FilterCommentsManager
@brief FilterCommentsManager reads language comment start and end marker strings from
		an INI file, and stores it in the map member variable m_FilterCommentsSetByFileType.
		Each set of comment markers have a list of file types that can be used with
		the file markers.
@note
The ignore-comment logic can only use ANSI strings, because the search buffer is
char* type.
FilterCommentsManager uses _T logic, only so-as to allow UNICODE file names to be 
used for the INI file, or INI file base directory.
After retrieving data from INI file, the data is converted to ANSI.
If no INI file exist, or the INI file is empty, then a default INI file is 
created with default values that are assoicated with most commen languages.
*/
class FilterCommentsManager
{
public:
	FilterCommentsManager();
	const FilterCommentsSet &GetSetForFileType(const String& FileTypeName) const;
private:
	FilterCommentsManager(const FilterCommentsManager&); //Don't allow copy
	FilterCommentsManager& operator=(const FilterCommentsManager&);//Don't allow assignment
	//Use String instead of std::string, so as to allow UNICODE file extensions
	FilterCommentsSet m_empty;
	stl::map<String, FilterCommentsSet> m_FilterCommentsSetByFileType;
};

#endif // _FILTERCOMMENTSMANAGER_H_
