/**
 * @file  DiffUtils.h
 *
 * @brief Declaration of DiffUtils class.
 */
// ID line follows -- this is updated by SVN
// $Id$


#ifndef _DIFF_UTILS_H_
#define _DIFF_UTILS_H_

class CompareOptions;
class FilterList;
struct file_data;
struct FileTextStats;
class FilterCommentsManager;
class CDiffWrapper;

#include "DiffWrapper.h"

namespace CompareEngines
{

/**
 * @brief A class wrapping diffutils as compare engine.
 *
 * This class needs to have all its data as local copies, not as pointers
 * outside. Lifetime can vary certainly be different from unrelated classes.
 * Filters list being an exception - pcre structs are too complex to easily
 * copy so we'll only keep a pointer to external list.
 */
class DiffUtils : public CDiffWrapper
{
public:
	DiffUtils(const DIFFOPTIONS &);
	~DiffUtils();
	void SetFileData(int items, file_data *data);
	int diffutils_compare_files();
	bool RegExpFilter(int StartPos, int EndPos, int FileNo);
	void GetDiffCounts(int &diffs, int &trivialDiffs);
	void GetTextStats(int side, FileTextStats *stats);
	bool Diff2Files(struct change **diffs, int depth,
			int *bin_status, bool bMovedBlocks, int *bin_file);
	void SetCodepage(int codepage) { m_codepage = codepage; }

private:
	file_data *m_inf; /**< Compared files data (for diffutils). */
	int m_ndiffs; /**< Real diffs found. */
	int m_ntrivialdiffs; /**< Ignored diffs found. */
};

} // namespace CompareEngines

#endif // _DIFF_UTILS_H_
