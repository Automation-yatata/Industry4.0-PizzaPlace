CREATE SCHEMA gman_a35;

CREATE TABLE section( section_id INT, CONSTRAINT PK_section PRIMARY KEY (section_id) );

CREATE TABLE mote ( mote_id INT, section_id INT NOT NULL, CONSTRAINT PK_mote PRIMARY KEY (mote_id));

CREATE TABLE sensor ( name VARCHAR(45), mote_id INT NOT NULL, CONSTRAINT PK_sens_name PRIMARY KEY (name));

CREATE TABLE actuator (name VARCHAR(45),section_id INT NOT NULL,CONSTRAINT PK_act_name PRIMARY KEY (name));

CREATE TABLE rule (rule_id INT, name VARCHAR(45) NOT NULL, act_state BOOLEAN, CONSTRAINT PK_rule PRIMARY KEY  (rule_id));

CREATE TABLE subrule( subrule_id INT, sensor_name VARCHAR(45) NOT NULL, operation VARCHAR(2) NOT NULL,ref REAL NOT NULL, CONSTRAINT PK_subRule PRIMARY KEY (subrule_id));

CREATE TABLE op_r_subr ( subrule_id INT, rule_id INT NOT NULL, op_between_rules VARCHAR(5),CONSTRAINT PK_op_r_subr PRIMARY KEY (subrule_id));

CREATE TABLE sensor_vec (date TIMESTAMP, value REAL NOT NULL ,name_sens VARCHAR(45), CONSTRAINT PK_sens_vec PRIMARY KEY (date,name_sens));

CREATE TABLE actuator_vec (date TIMESTAMP, name_act VARCHAR(45), state BOOLEAN NOT NULL, CONSTRAINT PK_act_vec PRIMARY KEY (date,name_act));

ALTER TABLE mote ADD CONSTRAINT FK_sectionID  FOREIGN KEY (section_id) REFERENCES section (section_id);

ALTER TABLE sensor ADD CONSTRAINT FK_moteID  FOREIGN KEY (mote_id) REFERENCES mote (mote_id) ;

ALTER TABLE actuator ADD CONSTRAINT FK_sectionID FOREIGN KEY (section_id) REFERENCES section (section_id) ;

ALTER TABLE rule ADD CONSTRAINT FK_RuleActName FOREIGN KEY (name) REFERENCES actuator (name) ;

ALTER TABLE subrule ADD CONSTRAINT FK_sensorName FOREIGN KEY (sensor_name) REFERENCES sensor (name);

ALTER TABLE op_r_subr ADD CONSTRAINT FK_subruleID FOREIGN KEY (subrule_id) REFERENCES subrule (subrule_id);

ALTER TABLE op_r_subr ADD CONSTRAINT FK_rule_id FOREIGN KEY (rule_id) REFERENCES rule (rule_id);

ALTER TABLE actuator_vec ADD CONSTRAINT FK_Name FOREIGN KEY (name_act) REFERENCES actuator (name);

ALTER TABLE sensor_vec ADD CONSTRAINT PK_Name  FOREIGN KEY (name_sens) REFERENCES sensor (name) ;

