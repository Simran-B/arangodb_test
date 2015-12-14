/*jshint globalstrict:false, strict:false */
/*global assertEqual */

////////////////////////////////////////////////////////////////////////////////
/// @brief test the correctness of a skip-list index
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2013 triagens GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Max Neunhoeffer
/// @author Copyright 2013, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

var jsunity = require("jsunity");
var internal = require("internal");

// -----------------------------------------------------------------------------
// --SECTION--                                                     basic methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief test suite: Creation
////////////////////////////////////////////////////////////////////////////////

function SkipListCorrSuite() {
  'use strict';
  var cn = "UnitTestsCollectionSkiplistCorr";
  var coll = null;
  var helper = require("@arangodb/aql-helper");
  var getQueryResults = helper.getQueryResults;

  return {

////////////////////////////////////////////////////////////////////////////////
/// @brief set up
////////////////////////////////////////////////////////////////////////////////

    setUp : function () {
      internal.db._drop(cn);
      coll = internal.db._create(cn);
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief tear down
////////////////////////////////////////////////////////////////////////////////

    tearDown : function () {
      // try...catch is necessary as some tests delete the collection itself!
      try {
        coll.unload();
        coll.drop();
      }
      catch (err) {
      }

      coll = null;
      internal.wait(0.0);
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test: performance of deletion with skip-list index
////////////////////////////////////////////////////////////////////////////////

    testCorrectnessNonUnique : function () {
      coll.ensureSkiplist("v");

      coll.save({a:1,_key:"1",v:1});
      coll.save({a:17,_key:"2",v:2});
      coll.save({a:22,_key:"3",v:3});
      coll.save({a:4,_key:"4",v:1});
      coll.save({a:17,_key:"5",v:2});
      coll.save({a:6,_key:"6",v:3});
      coll.save({a:12,_key:"7",v:4});
      coll.save({a:100,_key:"8",v:3});
      coll.save({a:7,_key:"9",v:3});

      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v == 3 RETURN x").length, 4);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 RETURN x").length, 4);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v <= 3 RETURN x").length, 8);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 3 RETURN x").length, 1);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v >= 3 RETURN x").length, 5);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 0 RETURN x").length, 9);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 10 RETURN x").length, 9);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 || x.v > 3 RETURN x").length,5);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 && x.v > 3 RETURN x").length,0);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 1 RETURN x").length, 0);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v <= 1 RETURN x").length, 2);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 4 RETURN x").length, 0);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v >= 4 RETURN x").length, 1);

      coll.removeByExample({v:3});

      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v == 3 RETURN x").length, 0);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 RETURN x").length, 4);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v <= 3 RETURN x").length, 4);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 3 RETURN x").length, 1);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v >= 3 RETURN x").length, 1);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 0 RETURN x").length, 5);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 10 RETURN x").length, 5);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 || x.v > 3 RETURN x").length,5);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 && x.v > 3 RETURN x").length,0);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 1 RETURN x").length, 0);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v <= 1 RETURN x").length, 2);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 4 RETURN x").length, 0);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v >= 4 RETURN x").length, 1);
    },

    testCorrectnessUnique : function () {
      coll.ensureUniqueSkiplist("v");

      coll.save({a:1,_key:"1",v:1});
      coll.save({a:17,_key:"2",v:2});
      coll.save({a:22,_key:"3",v:3});
      coll.save({a:4,_key:"4",v:-1});
      coll.save({a:17,_key:"5",v:5});
      coll.save({a:6,_key:"6",v:6});
      coll.save({a:12,_key:"7",v:4});
      coll.save({a:100,_key:"8",v:-3});
      coll.save({a:7,_key:"9",v:-7});

      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v == 3 RETURN x").length, 1);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 RETURN x").length, 5);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v <= 3 RETURN x").length, 6);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 3 RETURN x").length, 3);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v >= 3 RETURN x").length, 4);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 0 RETURN x").length, 6);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 10 RETURN x").length, 9);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 || x.v > 3 RETURN x").length,8);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 && x.v > 3 RETURN x").length,0);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 1 RETURN x").length, 3);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v <= 1 RETURN x").length, 4);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 4 RETURN x").length, 2);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v >= 4 RETURN x").length, 3);

      coll.removeByExample({v:3});

      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v == 3 RETURN x").length, 0);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 RETURN x").length, 5);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v <= 3 RETURN x").length, 5);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 3 RETURN x").length, 3);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v >= 3 RETURN x").length, 3);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 0 RETURN x").length, 5);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 10 RETURN x").length, 8);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 || x.v > 3 RETURN x").length,8);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 && x.v > 3 RETURN x").length,0);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 1 RETURN x").length, 3);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v <= 1 RETURN x").length, 4);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 4 RETURN x").length, 2);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v >= 4 RETURN x").length, 3);
    },

    testCorrectnessSparse : function () {
      coll.ensureUniqueSkiplist("v", { sparse: true });

      coll.save({a:1,_key:"1",v:1});
      coll.save({a:17,_key:"2",v:2});
      coll.save({a:22,_key:"3",v:3});
      coll.save({a:4,_key:"4",v:-1});
      coll.save({a:17,_key:"5",v:5});
      coll.save({a:6,_key:"6",v:6});
      coll.save({a:12,_key:"7",v:4});
      coll.save({a:100,_key:"8",v:-3});
      coll.save({a:7,_key:"9",v:-7});

      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v == 3 RETURN x").length, 1);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 RETURN x").length, 5);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v <= 3 RETURN x").length, 6);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 3 RETURN x").length, 3);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v >= 3 RETURN x").length, 4);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 0 RETURN x").length, 6);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 10 RETURN x").length, 9);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 || x.v > 3 RETURN x").length,8);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 && x.v > 3 RETURN x").length,0);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 1 RETURN x").length, 3);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v <= 1 RETURN x").length, 4);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 4 RETURN x").length, 2);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v >= 4 RETURN x").length, 3);

      coll.removeByExample({v:3});

      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v == 3 RETURN x").length, 0);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 RETURN x").length, 5);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v <= 3 RETURN x").length, 5);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 3 RETURN x").length, 3);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v >= 3 RETURN x").length, 3);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 0 RETURN x").length, 5);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 10 RETURN x").length, 8);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 || x.v > 3 RETURN x").length,8);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 3 && x.v > 3 RETURN x").length,0);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v < 1 RETURN x").length, 3);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v <= 1 RETURN x").length, 4);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v > 4 RETURN x").length, 2);
      assertEqual(getQueryResults(
                  "FOR x IN "+cn+" FILTER x.v >= 4 RETURN x").length, 3);
    }
  };
}

// -----------------------------------------------------------------------------
// --SECTION--                                                              main
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief executes the test suites
////////////////////////////////////////////////////////////////////////////////

jsunity.run(SkipListCorrSuite);

return jsunity.done();

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// @addtogroup\\|// --SECTION--\\|/// @page\\|/// @}"
// End:
