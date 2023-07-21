#ifndef CS564_PROJECT_PAGE_CACHE_LRU_2_HPP
#define CS564_PROJECT_PAGE_CACHE_LRU_2_HPP

#include "page_cache.hpp"
#include <list>
#include <unordered_map>

class LRU2ReplacementPageCache : public PageCache {
public:
  LRU2ReplacementPageCache(int pageSize, int extraSize);

  ~LRU2ReplacementPageCache() override;

  void setMaxNumPages(int maxNumPages) override;

  [[nodiscard]] int getNumPages() const override;

  Page *fetchPage(unsigned int pageId, bool allocate) override;

  void unpinPage(Page *page, bool discard) override;

  void changePageId(Page *page, unsigned int newPageId) override;

  void discardPages(unsigned int pageIdLimit) override;

private:
  struct LRU2ReplacementPage : public Page {
    LRU2ReplacementPage(int pageSize, int extraSize, unsigned pageId,
                        bool pinned);
    unsigned pageId;
    bool pinned;
    int references;
  };
  // stores the pagesID as the key and the page itself as the value
  std::unordered_map<unsigned, LRU2ReplacementPage *> pages_;
  // keeps and ordered list of pages from LRU to MRU
  std::list<LRU2ReplacementPage *> cache_; // TODO change to a better name
};

#endif // CS564_PROJECT_PAGE_CACHE_LRU_2_HPP
