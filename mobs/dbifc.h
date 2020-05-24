// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2020 Matthias Lautner
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

/** \file dbifc.h
 \brief Datenbank-Interface für Hersteller-unabhängigen Zugriff */

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedGlobalDeclarationInspection"
#ifndef MOBS_DBIFC_H
#define MOBS_DBIFC_H

#include "objgen.h"
#include <limits>
#include <chrono>
#include <utility>


namespace mobs {
  class DatabaseInterface;

  /// Cursor für sequentiellen Datenbankzugriff
  class DbCursor {
  public:
    virtual ~DbCursor() = default;
    /// Ende-Status
    virtual bool eof() = 0;
    /// Gültigkeit-Status
    virtual bool valid() = 0;
    /// Gültigkeit als Boolescher Operator
    explicit operator bool() { return valid(); }
    /// Increment-Operator
    virtual void operator++() = 0;
    /// Aktuelle Position, bei eof(), Anzahl der Datensätze
    size_t pos() const { return cnt; }
  protected:
    /// \private
    size_t cnt = 0;
  };

  /** \brief Interne Klasse, die alle Informationen zu einer Datenbankverbindung hält.
   *
   * \see DatabaseManager
   */
  class DatabaseConnection {
  public:
    virtual ~DatabaseConnection() = default;
    /// \private
    virtual bool load(DatabaseInterface &dbi, ObjectBase &obj) = 0;
    /// \private
    virtual void save(DatabaseInterface &dbi, const ObjectBase &obj) = 0;
    /// \private
    virtual bool destroy(DatabaseInterface &dbi, const ObjectBase &obj) = 0;
    /// \private
    virtual std::shared_ptr<DbCursor> query(DatabaseInterface &dbi, ObjectBase &obj, const std::string &query) = 0;
    /// \private
    virtual std::shared_ptr<DbCursor> qbe(DatabaseInterface &dbi, ObjectBase &obj) = 0;
    /// \private
    virtual void retrieve(DatabaseInterface &dbi, ObjectBase &obj, std::shared_ptr<mobs::DbCursor> cursor) = 0;

  };

  class ConnectionInformation {
  public:
    ConnectionInformation() = default;
    /// Informationen für eine Datenbankverbindung
    ConnectionInformation(std::string url, std::string database, std::string user = "", std::string password = "") :
                          m_url(std::move(url)), m_database(std::move(database)), m_user(std::move(user)),
                          m_password(std::move(password)) { }
    virtual ~ConnectionInformation() = default;

    std::string m_url; ///< URI
    std::string m_database; ///< Datenbankname
    std::string m_user; ///< Benutzername
    std::string m_password; ///< Passwort
  };


  class DatabaseInterface
  {
  public:
    /// \private
    DatabaseInterface(std::shared_ptr<DatabaseConnection> dbi, std::string dbName);

    /** Lade ein Objekt anhand der vorbesetzten Key-Elemente
     * \return true, wenn ein Objekt geladen werden konnte, false wenn kein passendes Objekt existiert
     * \throw runtime_error wenn ein Fehler auftrat
     */
    bool load(ObjectBase &obj);
//    virtual bool load(std::list<ObjectBase> &result, std::string objType, std::string query) = 0;
    /// Speichern eines Objektes in die Datenbank 
    void save(const ObjectBase &obj);
    /** Lösche ein Objekt anhand der vorbesetzten Key-Elemente
      * \return true, wenn das Objekt gelöscht werden konnte, false wenn kein passendes Objekt existiert
      * \throw runtime_error wenn ein Fehler auftrat
      */
    bool destroy(const ObjectBase &obj);

    /** \brief Datenbankabfrage über eine Bedingung als Text
     * 
     * Die Angabe der Bedingung ist datenbankabhänging
     * @param obj Objekt zur Generierung der Abfrage
     * @param query Bedingung/Filter der Abfrage
     * @return Shared-Pointer auf einen Ergebnis-Cursor  
     * \throw runtime_error wenn ein Fehler auftrat. Eine leere Ergebnismenge ist kein Fehler
     */
    std::shared_ptr<DbCursor> query(ObjectBase &obj, const std::string &query);

      /** \brief Datenbankabfrage Query By Example
       * 
       * Es wird eine Datenbankabfrage aus dem übergebenen Objekt generiert, die auf alle,
       * als "modified" gekennzeichneten Elemente passt.
       * @param obj Objekt zum Aufbau der Query
       * @return Shared-Pointer auf einen Ergebnis-Cursor  
       * \throw runtime_error wenn ein Fehler auftrat. Eine leere Ergebnismenge ist kein Fehler
       */
    std::shared_ptr<DbCursor> qbe(ObjectBase &obj);
    /** lade das Objekt, auf das der Cursor zeigt
     * 
     * @param obj Objekt das mit dem Inhalt der Datenbank überschrieben wird
     * @param cursor Cursor, der auf ein Objekt zeigt, valid() und nicht eof()
     * \throw runtime_error wenn ein Fehler auftrat
     */
    void retrieve(ObjectBase &obj, std::shared_ptr<mobs::DbCursor> cursor);

    /// liefert den Datenbanknamen des Interfaces
    const std::string &database() const noexcept { return databaseName; }

    /** \brief Gibt eine Referenz auf die Datenbank Connection zurück, um auf Treiberspezifische Funktionen zuzugreifen
     *
     * \code
     *   auto mdb = dynamic_pointer_cast<mongoDatabaseConnection>(dbInterface.getConnection());
     *   if (mdb)
     *     mdb->...
     * \endcode
     */
    std::shared_ptr<DatabaseConnection> getConnection() const noexcept { return dbCon; }

    /// Erzeuge ein Duplikat mit der Option einen Cursor zu liefern, der nur die Anzahl der gefundenen Elemente bereitstellt
    DatabaseInterface withCountCursor() { DatabaseInterface d(*this); d.countCursor = true; return d; }
    /// Erzeuge ein Duplikat mit der Option, die ersten skip Objekte einer Query zu überspringen
    DatabaseInterface withQuerySkip(size_t skipCnt) { DatabaseInterface d(*this); d.skip = skipCnt; return d; }
    /// Erzeuge ein Duplikat mit der Option, die Query nach limit Objekten abzubrechen
    DatabaseInterface withQueryLimit(size_t limitCnt) { DatabaseInterface d(*this); d.limit = limitCnt; return d; }
    /// Erzeuge ein Duplikat mit der Option, die Query nach limit Objekten abzubrechen
    DatabaseInterface withTimeout(std::chrono::milliseconds millisec) { DatabaseInterface d(*this); d.timeout = millisec; return d; }
    /// Abfrage countCursor
    bool getCountCursor() const { return countCursor; }
    /// Abfrage Timeout
    std::chrono::milliseconds getTimeout() const { return timeout; }
    /// Abfrage Query-Skip
    size_t getQuerySkip() const { return skip; }
    /// Abfrage Query-Limit
    size_t getQueryLimit() const { return limit; }

  private:
    std::shared_ptr<DatabaseConnection> dbCon;
    std::string databaseName;
    bool countCursor = false;
    size_t skip = 0;
    size_t limit = 0;
    std::chrono::milliseconds timeout;
  };



  /// \private
  class DatabaseManagerData;

  /** \brief Singleton-Klasse eins Verwalters aller Datenbanken.
   * Das Objekt muss genau ein mal instanziiert werden, am besten in main oder einer Basisfunktion.
   * Alle verfügbaren Datenbanken müssen einmalig am Manager angemeldet werden. Danach können die Zugriffe
   * über das bereitgestellte Interface erfolgen.
   * Der Zugriffsbereich muss im Gültigkeitsbereich von main liegen, da exceptions geworfen werden.
   * Am Ende des  Gültigkeitsbereichs des Managers werden alle Datenbanken geschlossen.
   *
   * \code
   * main() {
   *   mobs::DatabaseManager dbMgr;
   *   dbMgr.addConnection("mobs", mobs::ConnectionInformation("mongodb://localhost:27017", "mobs"));
   *   ...
   *   mobs::DatabaseInterface mongo = dbMgr.getDbIfc("mobs");
   * \endcode
 */
  class DatabaseManager {
  public:
    DatabaseManager();
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;
    /// globaler Zugriff auf den Manager, sofern initialisiert
    static DatabaseManager *instance() noexcept { return manager; };

    /** \brief Erzeuge eine (weitere) Verbindung zur Datenbank
     *
     * @param connectionName Applikation-interner Name für die Verbindung
     * @param connectionInformation Informationen zum Aufbau der Verbindung
     */
    void addConnection(const std::string &connectionName, const ConnectionInformation &connectionInformation);
    /** \brief Verwende einen vorhandenen Datenbankzugang mit abweichenden Datenbanknamen
     *
     * @param connectionName Applikation-interner Name für die Verbindung
     * @param oldConnectionName Name der Verbindung die verwendet werden soll
     * @param database neuer Datenbank-Name
     */
    void copyConnection(const std::string &connectionName, const std::string &oldConnectionName, const std::string &database);

    /// Erzeuge eine Kopie des Datenbank-Interfaces zu der angegebene Connection
    DatabaseInterface getDbIfc(const std::string &connectionName);

  private:
    static DatabaseManager *manager;
    DatabaseManagerData *data = nullptr;
  };
}

#endif //MOBS_DBIFC_H

#pragma clang diagnostic pop