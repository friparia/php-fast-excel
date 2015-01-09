#php-fast-excel

PHP fast read and write excel file extension
####It is still under development, so please be aware of the production environment

## Compare to PHPExcel
First one is a 500 * 20 - cell .xls file

![test1](http://file.friparia.com/test1.png)

Second one is a 60k * 20 - cell .xls file 

![test2](http://file.friparia.com/test2.png)

## Requirement
- PHP 5.4+

## Install
You can clone the project from github and compile and install.
```shell
$ git clone https://github.com/friparia/php-fast-excel
$ cd php-fast-excel
$ /path/to/phpize
$ ./configure
$ make && make install
```
And then, you need to add a line in your `php.ini` file
```ini
extension=fast_excel.so
```

`windows is not supported yet`

## Usage
Easily, you can read an array from the function `excel_get_array()` like this
```php
<?php
$array = excel_get_array('/path/to/your.xls');
var_dump($array);
//a two dimension array contain all the cells in
?>
```
## FAQS
Some functions are not currently supported like cell which have its own format and so on, it just support the string format.

## Questions

It is still under developing.So if you have any ideas or other questions, please let me know in issues!!

## Todo

- [ ] Read the other types of the cell

- [ ] Read the version below BIFF7 (7 included) 

- [ ] Read the .xlsx file

- [ ] Write the .xlsx file

