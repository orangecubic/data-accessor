SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0;
SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0;
SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION';

-- -----------------------------------------------------
-- Schema mydb
-- -----------------------------------------------------
DROP SCHEMA IF EXISTS `mydb` ;

-- -----------------------------------------------------
-- Schema mydb
-- -----------------------------------------------------
CREATE SCHEMA IF NOT EXISTS `mydb` DEFAULT CHARACTER SET utf8 ;
USE `mydb` ;

-- -----------------------------------------------------
-- Table `mydb`.`users`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mydb`.`users` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `nickname` VARCHAR(32) NOT NULL,
  `created_at` DATETIME(3) NOT NULL,
  `updated_at` DATETIME(3) NOT NULL,
  `deleted_at` DATETIME(3) NULL,
  PRIMARY KEY (`id`))
ENGINE = InnoDB;

CREATE UNIQUE INDEX `users_nickname_unique` ON `mydb`.`users` (`nickname` ASC) VISIBLE;


-- -----------------------------------------------------
-- Table `mydb`.`battles`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mydb`.`battles` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `setting` JSON NOT NULL,
  `started_at` DATETIME(3) NULL,
  `ended_at` DATETIME(3) NULL,
  `created_at` DATETIME(3) NOT NULL,
  `updated_at` DATETIME(3) NOT NULL,
  PRIMARY KEY (`id`))
ENGINE = InnoDB;


-- -----------------------------------------------------
-- Table `mydb`.`matching_requests`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mydb`.`matching_requests` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `user_id` BIGINT UNSIGNED NOT NULL,
  `dup_flag` TINYINT NULL,
  `battle_id` BIGINT UNSIGNED NULL,
  `status` TINYINT NOT NULL,
  `created_at` DATETIME(3) NOT NULL,
  `updated_at` DATETIME(3) NOT NULL,
  PRIMARY KEY (`id`),
  CONSTRAINT `matching_requests_user_id_key`
    FOREIGN KEY (`user_id`)
    REFERENCES `mydb`.`users` (`id`)
    ON DELETE RESTRICT
    ON UPDATE RESTRICT,
  CONSTRAINT `matching_requests_battle_id_key`
    FOREIGN KEY (`battle_id`)
    REFERENCES `mydb`.`battles` (`id`)
    ON DELETE CASCADE
    ON UPDATE RESTRICT)
ENGINE = InnoDB;

CREATE INDEX `matching_requests_battle_id_key` ON `mydb`.`matching_requests` (`battle_id` ASC) VISIBLE;

CREATE UNIQUE INDEX `matching_requests_dup_unique` ON `mydb`.`matching_requests` (`user_id` ASC, `dup_flag` ASC) VISIBLE;

CREATE INDEX `matching_requests_status_key` ON `mydb`.`matching_requests` (`status` ASC, `created_at` ASC) VISIBLE;

CREATE UNIQUE INDEX `matching_requests_user_and_id_key` ON `mydb`.`matching_requests` (`id` ASC, `user_id` ASC) VISIBLE;


-- -----------------------------------------------------
-- Table `mydb`.`battle_results`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mydb`.`battle_results` (
  `user_id` BIGINT UNSIGNED NOT NULL,
  `matching_request_id` BIGINT UNSIGNED NOT NULL,
  `battle_id` BIGINT UNSIGNED NOT NULL,
  `is_winner` TINYINT NOT NULL,
  `created_at` DATETIME(3) NOT NULL,
  `updated_at` DATETIME(3) NOT NULL,
  PRIMARY KEY (`user_id`, `created_at`, `matching_request_id`),
  CONSTRAINT `battle_results_matching_request_id_key`
    FOREIGN KEY (`matching_request_id` , `user_id`)
    REFERENCES `mydb`.`matching_requests` (`id` , `user_id`)
    ON DELETE RESTRICT
    ON UPDATE RESTRICT,
  CONSTRAINT `battle_results_battle_id_key`
    FOREIGN KEY (`battle_id`)
    REFERENCES `mydb`.`battles` (`id`)
    ON DELETE CASCADE
    ON UPDATE RESTRICT)
ENGINE = InnoDB;

CREATE INDEX `battle_results_matching_request_id_idx` ON `mydb`.`battle_results` (`user_id` ASC, `matching_request_id` ASC) VISIBLE;

CREATE INDEX `battle_results_battle_id_key` ON `mydb`.`battle_results` (`battle_id` ASC) VISIBLE;


-- -----------------------------------------------------
-- Table `mydb`.`user_authentications`
-- -----------------------------------------------------
CREATE TABLE IF NOT EXISTS `mydb`.`user_authentications` (
  `user_id` BIGINT UNSIGNED NOT NULL,
  `password` VARCHAR(1024) NOT NULL,
  `created_at` DATETIME(3) NOT NULL,
  `updated_at` DATETIME(3) NOT NULL,
  PRIMARY KEY (`user_id`),
  CONSTRAINT `user_authentication_user_id_key`
    FOREIGN KEY (`user_id`)
    REFERENCES `mydb`.`users` (`id`)
    ON DELETE CASCADE
    ON UPDATE CASCADE)
ENGINE = InnoDB;


SET SQL_MODE=@OLD_SQL_MODE;
SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS;
SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS;
