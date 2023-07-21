#ifndef CS564_PROJECT_PAGE_CACHE_LRU_HPP
#define CS564_PROJECT_PAGE_CACHE_LRU_HPP

#include "page_cache.hpp"
#include <list>
#include <unordered_map>

class LRUReplacementPageCache : public PageCache {
public:
  LRUReplacementPageCache(int pageSize, int extraSize);

  ~LRUReplacementPageCache() override;

  void setMaxNumPages(int maxNumPages) override;

  [[nodiscard]] int getNumPages() const override;

  Page *fetchPage(unsigned int pageId, bool allocate) override;

  void unpinPage(Page *page, bool discard) override;

  void changePageId(Page *page, unsigned int newPageId) override;

  void discardPages(unsigned int pageIdLimit) override;

private:
  struct LRUReplacementPage : public Page {
    LRUReplacementPage(int pageSize, int extraSize, unsigned pageId,
                       bool pinned);
    unsigned pageId;
    bool pinned;
  };
  // stores the pagesID as the key and the page itself as the value
  std::unordered_map<unsigned, LRUReplacementPage *> pages_;
  // keeps a list of unpinned pages from LRU to MRU
  std::list<LRUReplacementPage *> cache_;
};

#endif // CS564_PROJECT_PAGE_CACHE_LRU_HPP
