CREATE SCHEMA gman_a35;

CREATE TABLE rule (

    rule_id SERIAL,
    CONSTRAINT PK_rule PRIMARY KEY  (rule_id)
);

CREATE TABLE actuator (

    name VARCHAR(45),
    actual_state BOOLEAN NOT NULL, 
    CONSTRAINT PK_act_name PRIMARY KEY (name)
);

CREATE TABLE actuator_vec (

    date CURRENT_TIMESTAMP,
    state BOOLEAN NOT NULL,
    CONSTRAINT PK_act_vec PRIMARY KEY (date,name)
);

CREATE TABLE op_r_subr (

    op_between_rules VARCHAR(5),
    CONSTRAINT PRIMARY KEY (subrule_id) 
);

CREATE TABLE subrule(

    subrule_id SERIAL,
    operation VARCHAR(2) NOT NULL,
    ref NUMERIC(3,2) NOT NULL,
    CONSTRAINT PK_subRule PRIMARY KEY (subrule_id)
);


CREATE TABLE sensor (

    name VARCHAR(45),
    actual_value NUMERIC(3,2) NOT NULL ,
    CONSTRAINT PK_sens_name PRIMARY KEY (name)
);

CREATE TABLE sensor_vec (
    
    date CURRENT_TIMESTAMP,
    value NUMERIC(3,2) NOT NULL ,
    CONSTRAINT PK_sens_vec PRIMARY KEY (date,name)
);

CREATE TABLE mote (

    mote_id INT,
    CONSTRAINT PK_mote PRIMARY KEY (mote_id)
);

CREATE TABLE section(
    section_id INT,
    CONSTRAINT PK_section PRIMARY KEY (section_id)
);

ALTER TABLE rule ADD CONSTRAINT FK_RuleActName
    FOREIGN KEY (name) REFERENCES actuator (name) NOT NULL;

ALTER TABLE actuator_vec ADD CONSTRAINT FK_Name
    FOREIGN KEY (name) REFERENCES actuator(name);

ALTER TABLE r_subr ADD CONSTRAINT FK_ruleID 
    FOREIGN KEY (rule_id) REFERENCES rule (rule_id);

ALTER TABLE r_subr ADD CONSTRAINT FK_SubRuleID
    FOREIGN KEY (subrule_id) REFERENCES subrule (subrule_id);



