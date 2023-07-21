#include "page_cache_lru_2.hpp"
#include "utilities/exception.hpp"

LRU2ReplacementPageCache::LRU2ReplacementPage::LRU2ReplacementPage(
    int argPageSize, int argExtraSize, unsigned argPageId, bool argPinned)
    : Page(argPageSize, argExtraSize), pageId(argPageId), pinned(argPinned),
      references(0) {}

LRU2ReplacementPageCache::LRU2ReplacementPageCache(int pageSize, int extraSize)
    : PageCache(pageSize, extraSize) {}

LRU2ReplacementPageCache::~LRU2ReplacementPageCache() {
  for (auto &[pageId, page] : pages_) {
    free(page);
  }
}

void LRU2ReplacementPageCache::setMaxNumPages(int maxNumPages) {
  maxNumPages_ = maxNumPages;
  for (auto cacheIterator = cache_.begin(); cacheIterator != cache_.end();) {
    if (getNumPages() <= maxNumPages_)
      break;
    LRU2ReplacementPage *pageToRemove = *cacheIterator;
    if (pageToRemove->pinned) {
      cacheIterator++;
      continue;
    }
    // write tests that make sure we remove all occurrences of unpinned from the
    // cache
    // also make sure remove removes all elements equal to the element passed in
    cacheIterator = cache_.erase(
        std::remove(cache_.begin(), cache_.end(), pageToRemove), cache_.end());
    pages_.erase(pageToRemove->pageId);
    delete pageToRemove;
  }
}

int LRU2ReplacementPageCache::getNumPages() const { return (int)pages_.size(); }

Page *LRU2ReplacementPageCache::fetchPage(unsigned pageId, bool allocate) {
  ++numFetches_;
  auto pagesIterator = pages_.find(pageId);
  if (pagesIterator != pages_.end()) {
    ++numHits_;
    if (pagesIterator->second->references > 2 &&
        cache_.front()->pageId == pagesIterator->second->pageId) {
      cache_.pop_front();
      pagesIterator->second->references--;
    }
    pagesIterator->second->pinned = true;
    return pagesIterator->second;
  }

  if (!allocate)
    return nullptr;

  if (getNumPages() < maxNumPages_) {
    LRU2ReplacementPage *page =
        new LRU2ReplacementPage(pageSize_, extraSize_, pageId, true);
    pages_.emplace(pageId, page);
    return page;
  }

  // All pages are pinned. Return a null pointer.
  if (cache_.empty())
    return nullptr;
  LRU2ReplacementPage *pageToReplace = nullptr;
  auto cacheIterator = cache_.begin();
  // loop through array to see if a page with one reference exists
  while (cacheIterator != cache_.end()) {
    LRU2ReplacementPage *curr_page = *cacheIterator;
    if (curr_page->references == 1 && !curr_page->pinned) {
      pageToReplace = curr_page;
      break;
    }
    cacheIterator++;
  }
  // if there is a page with only one reference and unpinned, remove it
  /* otherwise, if all pages have 2 references, remove the LRU (head/front) page
   if unpinned */
  if (pageToReplace) {
    cache_.remove(pageToReplace);
    pages_.erase(pageToReplace->pageId);
  } else {
    pageToReplace = cache_.front();
    cache_.remove(pageToReplace);
    pages_.erase(pageToReplace->pageId);
  }
  // replace the page with the new data
  pageToReplace->pinned = true;
  pageToReplace->pageId = pageId;
  pageToReplace->references = 0;
  pages_.emplace(pageId, pageToReplace);
  return pageToReplace;
}

void LRU2ReplacementPageCache::unpinPage(Page *page, bool discard) {
  LRU2ReplacementPage *pageToUnpin = (LRU2ReplacementPage *)page;
  if (discard || getNumPages() > maxNumPages_) {
    LRU2ReplacementPage *pageToRemove = pageToUnpin;
    pages_.erase(pageToRemove->pageId);
    delete pageToRemove;
  } else {
    pageToUnpin->pinned = false;
    pageToUnpin->references++;     // increment the cache references
    cache_.push_back(pageToUnpin); // add newly unpinned page to back of list
  }
}

void LRU2ReplacementPageCache::changePageId(Page *page, unsigned newPageId) {
  LRU2ReplacementPage *pageToChange = (LRU2ReplacementPage *)page;
  pages_.erase(pageToChange->pageId);
  pageToChange->pageId = newPageId;

  auto [pagesIterator, success] = pages_.emplace(newPageId, pageToChange);
  if (!success) {
    delete pagesIterator->second;
    pagesIterator->second = pageToChange;
  }
}

void LRU2ReplacementPageCache::discardPages(unsigned pageIdLimit) {
  for (auto pagesIterator = pages_.begin(); pagesIterator != pages_.end();) {
    if (pagesIterator->second->pageId >= pageIdLimit) {
      cache_.remove(pagesIterator->second);
      delete pagesIterator->second;
      pagesIterator = pages_.erase(pagesIterator);
    } else {
      ++pagesIterator;
    }
  }
}
