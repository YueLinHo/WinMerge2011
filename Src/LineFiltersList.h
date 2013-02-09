/** 
 * @file  LineFiltersList.h
 *
 * @brief Declaration file for LineFiltersList class
 */
// ID line follows -- this is updated by SVN
// $Id$

#ifndef _LINEFILTERS_LIST_H_
#define _LINEFILTERS_LIST_H_

/**
 @brief Structure for one line filter.
 */
struct LineFilterItem
{
	int usage; /**< Is filter enabled? */
	HRESULT hr; /**< Is filter erroneous? */
	String filterStr; /**< Filter string */
	LineFilterItem() : usage(0), hr(S_OK) { }
};

/**
 @brief List of line filters.
 */
extern class LineFiltersList
{
	typedef stl::vector<LineFilterItem> t_items;
public:

	void AddFilter(LPCTSTR filter, int usage);
	t_items::size_type GetCount() const { return m_items.size(); }
	void Empty() { m_items.clear(); }
	LineFilterItem &GetAt(t_items::size_type i) { return m_items[i]; }
	bool Compare(LineFiltersList &);

	void LoadFilters();
	void SaveFilters();
	void swap(LineFiltersList &other) { m_items.swap(other.m_items); }

private:
	t_items m_items; /**< List for linefilter items */
} globalLineFilters;

#endif // _LINEFILTERS_LIST_H_
