-- Race/Class combo unlock (server side). Skills/spells apply automatically via
-- playercreateinfo_skills/_spell_custom race/class MASKS; only position (+ action bar) is per-combo.
-- Idempotent. Client also needs a CharBaseInfo.dbc patch to offer the combos in the UI.

-- GnomePriest (race 7, class 5)
DELETE FROM `playercreateinfo` WHERE `race`=7 AND `class`=5;
INSERT INTO `playercreateinfo` (`race`,`class`,`map`,`zone`,`position_x`,`position_y`,`position_z`,`orientation`)
  SELECT 7,5,`map`,`zone`,`position_x`,`position_y`,`position_z`,`orientation` FROM `playercreateinfo` WHERE `race`=7 AND `class`<>6 ORDER BY `class` LIMIT 1;
DELETE FROM `playercreateinfo_action` WHERE `race`=7 AND `class`=5;
INSERT INTO `playercreateinfo_action` (`race`,`class`,`button`,`action`,`type`)
  SELECT 7,5,`button`,`action`,`type` FROM `playercreateinfo_action` WHERE `race`=1 AND `class`=5;

-- HumanHunter (race 1, class 3)
DELETE FROM `playercreateinfo` WHERE `race`=1 AND `class`=3;
INSERT INTO `playercreateinfo` (`race`,`class`,`map`,`zone`,`position_x`,`position_y`,`position_z`,`orientation`)
  SELECT 1,3,`map`,`zone`,`position_x`,`position_y`,`position_z`,`orientation` FROM `playercreateinfo` WHERE `race`=1 AND `class`<>6 ORDER BY `class` LIMIT 1;
DELETE FROM `playercreateinfo_action` WHERE `race`=1 AND `class`=3;
INSERT INTO `playercreateinfo_action` (`race`,`class`,`button`,`action`,`type`)
  SELECT 1,3,`button`,`action`,`type` FROM `playercreateinfo_action` WHERE `race`=2 AND `class`=3;

-- DwarfShaman (race 3, class 7)
DELETE FROM `playercreateinfo` WHERE `race`=3 AND `class`=7;
INSERT INTO `playercreateinfo` (`race`,`class`,`map`,`zone`,`position_x`,`position_y`,`position_z`,`orientation`)
  SELECT 3,7,`map`,`zone`,`position_x`,`position_y`,`position_z`,`orientation` FROM `playercreateinfo` WHERE `race`=3 AND `class`<>6 ORDER BY `class` LIMIT 1;
DELETE FROM `playercreateinfo_action` WHERE `race`=3 AND `class`=7;
INSERT INTO `playercreateinfo_action` (`race`,`class`,`button`,`action`,`type`)
  SELECT 3,7,`button`,`action`,`type` FROM `playercreateinfo_action` WHERE `race`=2 AND `class`=7;

-- OrcMage (race 2, class 8)
DELETE FROM `playercreateinfo` WHERE `race`=2 AND `class`=8;
INSERT INTO `playercreateinfo` (`race`,`class`,`map`,`zone`,`position_x`,`position_y`,`position_z`,`orientation`)
  SELECT 2,8,`map`,`zone`,`position_x`,`position_y`,`position_z`,`orientation` FROM `playercreateinfo` WHERE `race`=2 AND `class`<>6 ORDER BY `class` LIMIT 1;
DELETE FROM `playercreateinfo_action` WHERE `race`=2 AND `class`=8;
INSERT INTO `playercreateinfo_action` (`race`,`class`,`button`,`action`,`type`)
  SELECT 2,8,`button`,`action`,`type` FROM `playercreateinfo_action` WHERE `race`=1 AND `class`=8;

-- TrollWarlock (race 8, class 9)
DELETE FROM `playercreateinfo` WHERE `race`=8 AND `class`=9;
INSERT INTO `playercreateinfo` (`race`,`class`,`map`,`zone`,`position_x`,`position_y`,`position_z`,`orientation`)
  SELECT 8,9,`map`,`zone`,`position_x`,`position_y`,`position_z`,`orientation` FROM `playercreateinfo` WHERE `race`=8 AND `class`<>6 ORDER BY `class` LIMIT 1;
DELETE FROM `playercreateinfo_action` WHERE `race`=8 AND `class`=9;
INSERT INTO `playercreateinfo_action` (`race`,`class`,`button`,`action`,`type`)
  SELECT 8,9,`button`,`action`,`type` FROM `playercreateinfo_action` WHERE `race`=1 AND `class`=9;

-- UndeadPaladin (race 5, class 2)
DELETE FROM `playercreateinfo` WHERE `race`=5 AND `class`=2;
INSERT INTO `playercreateinfo` (`race`,`class`,`map`,`zone`,`position_x`,`position_y`,`position_z`,`orientation`)
  SELECT 5,2,`map`,`zone`,`position_x`,`position_y`,`position_z`,`orientation` FROM `playercreateinfo` WHERE `race`=5 AND `class`<>6 ORDER BY `class` LIMIT 1;
DELETE FROM `playercreateinfo_action` WHERE `race`=5 AND `class`=2;
INSERT INTO `playercreateinfo_action` (`race`,`class`,`button`,`action`,`type`)
  SELECT 5,2,`button`,`action`,`type` FROM `playercreateinfo_action` WHERE `race`=1 AND `class`=2;
