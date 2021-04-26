CREATE SCHEMA gman;

CREATE TABLE rule (

    rule_id SERIAL,
    CONSTRAINT PK_rule PRIMARY KEY  (rule_id)
);

CREATE TABLE actuator (

    name VARCHAR(45),
    actual_state BOOLEAN, 
    CONSTRAINT PK_act_name PRIMARY KEY (name)
);

CREATE TABLE actuator_vec (

    date CURRENT_TIMESTAMP,
    state BOOLEAN,
    CONSTRAINT PK_act_vec PRIMARY KEY (date)
);

CREATE TABLE r_subr (

    op_between_rules VARCHAR(5)
);

CREATE TABLE sensor (

    name VARCHAR(45),
    actual_value NUMERIC(3,2),
    CONSTRAINT PK_sens_name PRIMARY KEY (name)
);

CREATE TABLE sensor_vec (
    
    date CURRENT_TIMESTAMP,
    value NUMERIC(3,2),
    CONSTRAINT PK_sens_vec PRIMARY KEY (date)
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

ALTER TABLE 
