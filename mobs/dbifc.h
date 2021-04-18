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
#include <functional>
#include <memory>
#include <iostream>



namespace mobs {
class DatabaseInterface;
class DbTransaction;
class DatabaseManager;
class QueryOrder;
class QueryGenerator;

/** \brief Exception falls Datenbank temporär geblockt oder nicht verfügbar
 *
 */
class locked_error : public std::runtime_error {
public:
  /// \private
  explicit locked_error(const std::string &e) : std::runtime_error(e) {}
};

/** \brief Exception falls bei save(..) oder destroy() der optimistic lock nicht geklappt hat
 *
 * für optimistic locking muss ein Versions-Element vorhanden sein
 */
 class optLock_error : public std::runtime_error {
public:
  /// \private
  explicit optLock_error(const std::string &e) : std::runtime_error(e) {}
};

/** \brief Exception falls Datenbank beim save(..) einen unique constraint verletzt
 *
 */
 class duplicateValue_error : public std::runtime_error {
public:
  /// \private
  explicit duplicateValue_error(const std::string &e) : std::runtime_error(e) {}
};

/// \private
class TransactionDbInfo {

};

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

  /// lade nächstes Element analog operator++
  void next() { operator++(); }

  /// Aktuelle Position, bei eof() oder withCountCursor, Anzahl der Datensätze
  size_t pos() const { return cnt; }

  /// ist Cursor im KeysOnly-Modus
  virtual bool keysOnly() const { return false; }

protected:
  /// \private
  size_t cnt = 0;
};



//Interne Basis-Klasse, alle Datenbankschnittstellen basieren darauf.
/// \private
class DatabaseConnection {
public:
  virtual ~DatabaseConnection() = default;

  /// Name des DB-Interfaces
  virtual std::string connectionType() const = 0;

  /// \private
  virtual bool load(DatabaseInterface &dbi, ObjectBase &obj) = 0;

  /// \private
  virtual void save(DatabaseInterface &dbi, const ObjectBase &obj) = 0;

  /// \private
  virtual bool destroy(DatabaseInterface &dbi, const ObjectBase &obj) = 0;

  /// \private
  virtual void dropAll(DatabaseInterface &dbi, const ObjectBase &obj) = 0;

  /// \private
  virtual void structure(DatabaseInterface &dbi, const ObjectBase &obj) = 0;

  /// \private
  virtual std::shared_ptr<DbCursor>
  query(DatabaseInterface &dbi, ObjectBase &obj, bool qbe, const QueryGenerator *query, const QueryOrder *sort) = 0;

  /// \private
  virtual void retrieve(DatabaseInterface &dbi, ObjectBase &obj, std::shared_ptr<mobs::DbCursor> cursor) = 0;

  /// \private
  virtual void startTransaction(DatabaseInterface &dbi, DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) = 0;
  /// \private
  virtual void endTransaction(DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) = 0;
  /// \private
  virtual void rollbackTransaction(DbTransaction *transaction, std::shared_ptr<TransactionDbInfo> &tdb) = 0;

  /// \private
  virtual size_t maxAuditChangesValueSize(const DatabaseInterface &dbi) const = 0;

  /** \brief BLOB mittels vorhandener Id in Datenbank ablegen
   *
   * Diese Funktion ist derzeit nur bei mongoDb implementiert
   * @param dbi DatabaseInterface
   * @param id Id des neuen Blobs (datenbankspezifisch)
   * @param source Stream aus dem die Datei gelesen wird
   * \throws exception im Fehlerfall
   */
  virtual void uploadFile(DatabaseInterface &dbi, const std::string &id, std::istream &source);

  /** \brief BLOB in Datenbank ablegen
   *
   * Diese Funktion ist derzeit nur bei mongoDb implementiert
   * @param dbi DatabaseInterface
   * @param source Stream aus dem die Datei gelesen wird
   * @return id, unter der die Datei abgelegt wurde
   * \throws exception im Fehlerfall
   */
  virtual std::string uploadFile(DatabaseInterface &dbi, std::istream &source);

  /** \brief BLOB aus Datenbank zurücklesen
   *
   * Diese Funktion ist derzeit nur bei mongoDb implementiert
   * @param dbi DatabaseInterface
   * @param id Id des BLOBs
   * @param dest
   * \throws exception im Fehlerfall
   */
  virtual void downloadFile(DatabaseInterface &dbi, const std::string &id, std::ostream &dest);

  /** \brief BLOB aus Datenbank löschen
   *
   * Diese Funktion ist derzeit nur bei mongoDb implementiert
   * @param dbi DatabaseInterface
   * @param id Id des BLOBs
   * \throws exception im Fehlerfall
   */
  virtual void deleteFile(DatabaseInterface &dbi, const std::string &id);
};

/// Container für die Information zu einer Datenbankverbindung
class ConnectionInformation {
public:
  ConnectionInformation() = default;

  /// Informationen für eine Datenbankverbindung
  ConnectionInformation(std::string url, std::string database, std::string user = "", std::string password = "") :
          m_url(std::move(url)), m_database(std::move(database)), m_user(std::move(user)),
          m_password(std::move(password)) {}

  virtual ~ConnectionInformation() = default;

  std::string m_url; ///< URI
  std::string m_database; ///< Datenbankname
  std::string m_user; ///< Benutzername
  std::string m_password; ///< Passwort
};

/** \brief Interface um Objekte in Datenbanken zu verwalten
 *
 * Das Interface wird vom DatabaseManager abgerufen.
 * \see mobs::DatabaseManager
 * Mit Hilfe des Interfaces können Optionen angepasst werden Objekte geladen, gespeichert, gesucht uvm. werden.
 * \code
 * mobs::DatabaseInterface dbi = dbMgr.getDbIfc("my_maria");
 * MobsObjekt f1;
 * ...
 * dbi.save(f1);
 * \endcode
 */
class DatabaseInterface {
public:
  friend class DbTransaction;

  /// \private
  DatabaseInterface(std::shared_ptr<DatabaseConnection> dbi, std::string dbName);

  /// liefert den Connection-Namen unter dem das Interface von DbManager erzeugt wurde
  std::string connectionName() const;

  /** Lade ein Objekt anhand der vorbesetzten Key-Elemente
   * \return true, wenn ein Objekt geladen werden konnte, false wenn kein passendes Objekt existiert
   * \throw runtime_error wenn ein Fehler auftrat
   */
  bool load(ObjectBase &obj);

  /** \brief Speichert ein Objekt in die Datenbank
   *
   * die Modified-Flags werden dabei zurückgesetzt und, falls vorhanden, das Versions-Feld hochgezählt.
   * Ist, falls vorhanden, das Versions die Objektes nicht identisch mit dem der Datenbank, wird eine
   * Exception geworfen
   * @param obj
   * \throw exception, bei Datenbankfehler oder Versions-mismatch
   */
  void save(ObjectBase &obj);

  /** \brief Speichert ein Objekt in die Datenbank (const)
   *
   * Modified-Flags und das Versions-Feld bleiben unverändert.
   * Ist, falls vorhanden, die Versions des Objektes nicht identisch mit dem der Datenbank, wird eine
   * Exception geworfen
   * @param obj
   * \throw exception, bei Datenbankfehler oder Versions-mismatch
   */
  void save(const ObjectBase &obj);

  /** \brief Lösche ein Objekt anhand der vorbesetzten Key-Elemente
   *
   * Modified-Flags und das Versions-Feld bleiben unverändert.
   * Ist, falls vorhanden, die Versions des Objektes nicht identisch mit dem der Datenbank, wird eine
   * Exception geworfen.
   * Hat das Objekt ein VersionsFeld, so muss dieses explizit zurückgesetzt (=0) werden, um wieder gespeichert werden zu können
   * \return true, wenn das Objekt gelöscht werden konnte, false wenn kein passendes Objekt (ohne VersionsFeld) existiert
   * \throw runtime_error wenn ein Fehler auftrat oder die Versionsinformation nicht passt, falls vorhanden
   */
  bool destroy(const ObjectBase &obj);

  /** \brief Lösche die gesamte Tabelle/Collection zu diesem Objekt -- Vorsicht was weg ist ist weg
    * \throw runtime_error wenn ein Fehler auftrat
    */
  void dropAll(const ObjectBase &obj);

  /** \brief Lege die Struktur der Tabelle/Collection zu diesem Objekt an, falls noch nicht existent
    * \throw runtime_error wenn ein Fehler auftrat
    */
  void structure(const ObjectBase &obj);

  /** \brief Datenbankabfrage über eine Bedingung als Text
   *
   * Die Angabe der Bedingung ist datenbankabhängig
   * @param obj Objekt zur Generierung der Abfrage
   * @param query Bedingung/Filter der Abfrage. \see QueryGenerator
   * @return Shared-Pointer auf einen Ergebnis-Cursor
   * \throw runtime_error wenn ein Fehler auftrat. Eine leere Ergebnismenge ist kein Fehler
   */
  std::shared_ptr<DbCursor> query(ObjectBase &obj, const QueryGenerator &query);

  /** \brief Datenbankabfrage über eine Bedingung als Text
  *
  * Die Angabe der Bedingung ist datenbankabhängig
  * @param obj Objekt zur Generierung der Abfrage
  * @param query Bedingung/Filter der Abfrage. \see QueryGenerator
  * @param sort Sortier-Objekt zum Aufbau der Query
  * @return Shared-Pointer auf einen Ergebnis-Cursor
  * \throw runtime_error wenn ein Fehler auftrat. Eine leere Ergebnismenge ist kein Fehler
  */
  std::shared_ptr<DbCursor> query(ObjectBase &obj, const QueryGenerator &query, const QueryOrder &sort);

  /** \brief Datenbankabfrage Query By Example
   *
   * Es wird eine Datenbankabfrage aus dem übergebenen Objekt generiert, die auf alle,
   * als "modified" gekennzeichneten Elemente passt.
   * @param obj Objekt zum Aufbau der Query
   * @return Shared-Pointer auf einen Ergebnis-Cursor
   * \throw runtime_error wenn ein Fehler auftrat. Eine leere Ergebnismenge ist kein Fehler
   */
  std::shared_ptr<DbCursor> qbe(ObjectBase &obj);

  /** \brief Datenbankabfrage Query By Example
   *
   * Es wird eine Datenbankabfrage aus dem übergebenen Objekt generiert, die auf alle,
   * als "modified" gekennzeichneten Elemente passt.
   * @param obj Objekt zum Aufbau der Query
   * @param sort Sortier-Objekt zum Aufbau der Query
   * @return Shared-Pointer auf einen Ergebnis-Cursor
   * \throw runtime_error wenn ein Fehler auftrat. Eine leere Ergebnismenge ist kein Fehler
   */
  std::shared_ptr<DbCursor> qbe(ObjectBase &obj, const QueryOrder &sort);

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
   *   auto mdb = dynamic_pointer_cast<MongoDatabaseConnection>(dbInterface.getConnection());
   *   if (mdb)
   *     mdb->...
   * \endcode
   */
  std::shared_ptr<DatabaseConnection> getConnection() const noexcept { return dbCon; }

  /// Erzeuge ein Duplikat mit der Option einen Cursor zu liefern, der nur die Anzahl der gefundenen Elemente bereitstellt
  DatabaseInterface withCountCursor() {
    DatabaseInterface d(*this);
    d.countCursor = true;
    return d;
  }

  /// Erzeuge ein Duplikat mit der Option \c dirtyRead oder ReadUncommitted um keine Locks zu erzeugen
  DatabaseInterface withDirtyRead() {
    DatabaseInterface d(*this);
    d.dirtyRead = true;
    return d;
  }

  /// Erzeuge ein Duplikat mit der Option \c keysOnly mit der nur noch die Key-Elemente bei einer Query gelesen werden
  DatabaseInterface withKeysOnly() {
    DatabaseInterface d(*this);
    d.keysOnly = true;
    return d;
  }

  /// Erzeuge ein Duplikat mit der Option, die ersten skip Objekte einer Query zu überspringen
  DatabaseInterface withQuerySkip(size_t skipCnt) {
    DatabaseInterface d(*this);
    d.skip = skipCnt;
    return d;
  }

  /// Erzeuge ein Duplikat mit der Option, die Query nach limit Objekten abzubrechen
  DatabaseInterface withQueryLimit(size_t limitCnt) {
    DatabaseInterface d(*this);
    d.limit = limitCnt;
    return d;
  }

  /// Erzeuge ein Duplikat mit der Option, die Query nach limit Objekten abzubrechen
  DatabaseInterface withTimeout(std::chrono::milliseconds millisec) {
    DatabaseInterface d(*this);
    d.timeout = millisec;
    return d;
  }

  /// Abfrage countCursor
  bool getCountCursor() const { return countCursor; }

  /// Abfrage Timeout
  std::chrono::milliseconds getTimeout() const { return timeout; }

  /// Abfrage Query-Skip
  size_t getQuerySkip() const { return skip; }

  /// Abfrage Query-Limit
  size_t getQueryLimit() const { return limit; }

  /// Abfrage DirtyRead
  bool getDirtyRead() const { return dirtyRead; }

  /// Abfrage keys only
  bool getKeysOnly() const { return keysOnly; }

  /// Abfrage Transaktion
  DbTransaction *getTransaction() const { return transaction; }

  /// Abfrage TransactionDbInfo
  TransactionDbInfo *transactionDbInfo() const;

  /// Abfrage max. Elementgröße AuditChanges-Value; 0 == unbegrenzt
  size_t maxAuditChangesValueSize() const;

private:
  std::shared_ptr<DatabaseConnection> dbCon;
  std::string databaseName;
  bool countCursor = false;
  bool keysOnly = false;
  bool dirtyRead = false;
  size_t skip = 0;
  size_t limit = 0;
  std::chrono::milliseconds timeout;
  DbTransaction *transaction = nullptr;
};



/// \private
class DatabaseManagerData;

/** \brief Singleton-Klasse eins Verwalters aller Datenbanken.
 *
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
  friend class DatabaseInterface;
  /// Call-Back für Transaktionsbehandlung
  using transaction_callback = std::function<void(DbTransaction *)>;

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
  void
  copyConnection(const std::string &connectionName, const std::string &oldConnectionName, const std::string &database);

  /// Erzeuge eine Kopie des Datenbank-Interfaces zu der angegebene Connection
  DatabaseInterface getDbIfc(const std::string &connectionName);

  /** \brief Führt die eigentliche Transaktion aus
   *
   * \see DbTransaction
   * @param cb Transaction Call-Back Funktion
   * \throw runtime_error, wenn die Transaktion nicht erfolgreich war
   */
  static void execute(transaction_callback &cb);

private:
  static DatabaseManager *manager;
  DatabaseManagerData *data = nullptr;
};


class DbTransactionData;

/** \brief Interface für Datenbank Transaktionen; Verwendung über Transaction-Callback-Funktion
  *
  * Wird innerhalb einer Call-Back Funktion für Transaktionen verwendet um auf ein
  * Datenbank-Interface zuzugreifen.
  * Die Transaktion wird als Lambda-Funktion dann dem DatabaseManager zur Ausführung übergeben.
  * Eine Exception in der Lambda-Funktion löst automatisch einen Rollback aus.
  * \code
  * mobs::DatabaseManager::transaction_callback transCb = [&f1](mobs::DbTransaction *trans) {
  *   // DatabaseInterface muss erneut, vom Transaktionsinterface, geholt werden
  *   mobs::DatabaseInterface t_dbi = trans->getDbIfc("my_database");
  *   t_dbi.destroy(f1);
  *   ...
  * };
  *
  * // Transaktion ausführen
  * mobs::DatabaseManager::execute(transCb);
  * \endcode
  */
class DbTransaction {
public:
  friend class DatabaseManager;
  friend class DatabaseInterface;
  /// Verwende Mikrosekunden als Auflösung für Audit-Trail
  using MTime = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;

  /// Konstanten für Isolation-Level
  enum IsolationLevel {
    ReadUncommitted, ReadCommitted, CursorStability, RepeatableRead, Serializable
  };

  /** \brief Erzeuge eine Kopie des Datenbank-Interfaces zu der angegebene Connection innerhalb der Transaktion
   *
   * @param connectionName analog DatabaseManager
   * @return neues Dbi für die Transaktion
   * \throws runtime_error wenn kein Datenbankmanager instanziiert oder der Name unbekannt ist
   */
  DatabaseInterface getDbIfc(const std::string &connectionName);

  /// Abfrage TransactionDbInfo
  TransactionDbInfo *transactionDbInfo(const DatabaseInterface &dbi);
  /// Startzeitpunkt der Transaktion
  MTime startTime() const;
  /// Abfrage Isolation-Level
  IsolationLevel getIsolation() const;

  /// Setzen des Isolation-Levels
  void setIsolation(IsolationLevel level);
  /// Setzen einer uid für Audit Trail; standardmäßig mit UserId des Systems vorbesetzt
  static void setUid(int id);
  /// Id des gesamten Prozesses, wird mit einer UUID vorbelegt
  static void setJobId(const std::string &id);
  /// Auslesen der JobId, ist mit einer UUID vorbelegt
  static const std::string &getJobId();
  /// setze Kommentar für Audit Trail
  void setComment(const std::string &comment);


private:
  DbTransaction();
  ~DbTransaction();
  DatabaseInterface getDbIfc(DatabaseInterface &dbi);
  void finish(bool good);
  void doAuditSave(const ObjectBase &obj, const DatabaseInterface &dbi);
  void doAuditDestroy(const ObjectBase &obj, const DatabaseInterface &dbi);
  void writeAuditTrail();

  DbTransactionData *data = nullptr;

};

}
#endif //MOBS_DBIFC_H

#pragma clang diagnostic pop
