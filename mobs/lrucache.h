// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2024 Matthias Lautner
//
// This is part of MObs https://github.com/AlMarentu/MObs.git
//
// MObs is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef MOBS_LRUCACHE_H
#define MOBS_LRUCACHE_H


#include "objgen.h"

#include <memory>
#include <string>
using u_int64_t = uint64_t;

namespace mobs {

/** \brief Klasse zum Cachen von beliebigen Objekten (T) über einen Schlüssel (K)
 *
 * Die Elemente werden über shared pointer verwaltet.
 * Wird ein Caxhe-Element gelöscht oder verdrängt, wso wird nur der shared_ptr aus dem Cache entfernt.
 * Die shared_ptr auf das verdrängte Objekt bleiben unverändert
 *
 * Die Verdrängung erfolgt nach dem last recent used Verfahren.
 */
template<class T, class K = std::string, class Compare = std::less<K>>
class LRUCache
{
public:
  LRUCache() = default;
  ~LRUCache() = default;
  LRUCache(const LRUCache &) = delete;
  LRUCache &operator=(const LRUCache &) = delete;

  /** \brief Objekt in Cache einfügen
   *
   * Ist der Schlüssel bereits vorhanden, so wird das Objekt ersetzt;
   * @param key Eindeutiger Schlüssel
   * @param p shared_ptr auf das Objekt
   * @param size Größe des Objektes in Bytes, kann entfallen, falls keine Verdrängung nach Speicherbedarf gewünscht
   * \throws runtime_error wenn ein Speicher-Fehler auftritt
   */
  void insert(K key, std::shared_ptr<T> p, size_t size = 0);
  /** \brief prüfen, ob ein Schlüssel vorhanden ist.
   *
   * das Prüfen auf Existentz wird nicht als Zugriff gezählt
   * @param key  Eindeutiger Schlüssel
   * @return true, wenn Schlüssel vorhanden
   */
  bool exists(const K &key);
  /** \brief Objekt aus Cache verwenden
   *
   * @param key Eindeutiger Schlüssel
   * @return shared pointer auf das Objekt, falls nicht vorhanden wird ein leerer Zeiger geliefert
   */
  std::shared_ptr<T> lookup(const K &key);
  /** \brief Löschen eines Cache-Elementes
   *
   * Ist das Objekt nicht vorhanden, so wird keine Aktion ausgeführt;
   * @param key Eindeutiger Schlüssel
   * \throws runtime_error wenn eine Inkonsistenz festgestellt wird
   */
  void erase(const K &key);
  /** \brief reduziere Cache auf Anzahl n
  *
  * die Verdrängung erfolgt nach last recent used
  * @param n ist n größer als die Cache-Size, so wird nur die Cache-Size zurückgeliefert
  * @return tatsächliche Größe des Caches
  */
  size_t reduceCount(size_t n);
  /** \brief reduziere Cache auf Größe n in Bytes
   *
   * Voraussetzung ist, dass beim insert die Größenangabe mitgegeben wurde.
   *
   * die Verdrängung erfolgt nach last recent used
   * @param n ist n größer als die Cache-Size, so wird nur die Cache-Size zurückgeliefert
   * @return tatsächliche Größe des Caches in Bytes
  */
  size_t reduceBytes(size_t n);


protected:
  class Info {
  public:
    explicit Info(std::shared_ptr<T> p, u_int64_t cnt, size_t s) : ptr(std::move(p)), pos(cnt), size(s) { }
    std::shared_ptr<T> ptr;
    u_int64_t pos;
    size_t size;
  };
  void used(Info &info);
  // um ein Element verkleinern
  void reduce();
  u_int64_t cnt = 0;
  size_t bytes = 0; // größe in Bytes
  std::map<K, Info, Compare> cache;
  std::map<u_int64_t, K> lru;
};

template<class T, class K, class Compare>
void LRUCache<T, K, Compare>::reduce() {
  auto lit = lru.begin();
  if (lit == lru.end())
    return;
  auto it = cache.find(lit->second);
  if (it == cache.end())
    return;
  if (bytes >= it->second.size)
    bytes -= it->second.size;
  else
    bytes = 0;
  lru.erase(lit);
  cache.erase(it);
}

/*********** implementation *************/

template<class T, class K, class Compare>
void LRUCache<T, K, Compare>::used(LRUCache::Info &info) {
  auto it = lru.find(info.pos);
  if (it == lru.end())
    THROW("cache is inconsistent");
  info.pos = ++cnt;
  lru.emplace(info.pos, it->second);
  lru.erase(it);
}

template<class T, class K, class Compare>
size_t LRUCache<T, K, Compare>::reduceBytes(size_t n) {
  while (bytes > n)
    reduce();
  return bytes;
}

template<class T, class K, class Compare>
size_t LRUCache<T, K, Compare>::reduceCount(size_t n) {
  while (lru.size() > n)
    reduce();
  return lru.size();
}

template<class T, class K, class Compare>
void LRUCache<T, K, Compare>::erase(const K &key) {
  auto it = cache.find(key);
  if (it == cache.end())
    return;
  if (bytes >= it->second.size)
    bytes -= it->second.size;
  else
    bytes = 0;
  lru.erase(it->second.pos);
  cache.erase(it);
}

template<class T, class K, class Compare>
bool LRUCache<T, K, Compare>::exists(const K &key) {
  auto it = cache.find(key);
  return it != cache.end();
}

template<class T, class K, class Compare>
std::shared_ptr<T> LRUCache<T, K, Compare>::lookup(const K &key) {
  auto it = cache.find(key);
  if (it == cache.end())
    return {};
  used(it->second);
  return it->second.ptr;
}

template<class T, class K, class Compare>
void LRUCache<T, K, Compare>::insert(K key, std::shared_ptr<T> p, size_t size) {
  auto it = cache.find(key);
  if (it == cache.end()) {
    it = cache.emplace(key, Info(std::move(p), ++cnt, size)).first;
    if (it == cache.end())
      THROW("can't save " << key);
    lru[it->second.pos] = std::move(key);
  } else {
    it->second.ptr = std::move(p);
    used(it->second);
    if (bytes >= it->second.size)
      bytes -= it->second.size;
    else
      bytes = 0;
  }
  bytes += size;
}

}

#endif // MOBS_LRUCACHE_H