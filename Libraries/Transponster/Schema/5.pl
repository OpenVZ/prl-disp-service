#!/usr/bin/perl -w

use strict;
use File::Spec;
use XML::LibXML;
use Data::Dumper;
use File::Basename;
use UNIVERSAL qw(isa);

package Strings;
use Carp qw<longmess>;
use Data::Dumper;
use Storable qw<freeze thaw>;
use MIME::Base64;
our @data = ('');
our $license = <<'LICENSE';
/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */
LICENSE
our @history_;
our $historyCursor = 0;
our $reserved = 0;

sub generate($$)
{
	my $n = $_[1];
	my $t = $n.($historyCursor - 1);
	$history_[$historyCursor] ne $t and $t = $n.$#data;
	return $_[0]->reverse($t);
}

sub lookup($$)
{
	print Dumper(longmess()) unless defined $_[1];
	return $data[$_[1]];
}

sub reverse($$)
{
	my ($z, $n) = @_;
	return undef unless defined($n) && $n gt '';
	my $output;
	if (@history_ > $historyCursor && $history_[$historyCursor] eq $n)
	{
		$output = $historyCursor++;
		$history_[$output] = undef;
		++$reserved;
	}
	else
	{
		($output) = grep {$data[$_] eq $n} 1..$#data;
		unless (defined $output)
		{
			++$reserved;
			$output = scalar(@data);
			push @data, $n;
		}
	}

	return $output;
}

sub getDefinition($)
{
	my @output;
	push @output, 'static QString g_text[] = {';
	push @output, map {qq(\tQString("$data[$_]"),)} 1..$#data - 1;
	push @output, qq(\tQString("$data[$#data]"));
	push @output, '};';
	push @output, '';
	push @output, 'const QString& getText(quint32 index_)';
	push @output, '{';
	push @output, "\t".'return g_text[index_ - 1];';
	push @output, '}';
	push @output, '';
	print STDERR 'ERROR: ', ($reserved - $historyCursor), $/;
	return @output;
}

sub prime($$)
{
	my $file = $_[1];
	open(FD, '<', $file) or die "Cannot open file $file: $!$/";
	@data = ('');
	@history_ = ('');
	$reserved = 1;
	$historyCursor = 1;
	while (my $l = <FD>)
	{
		if ($l =~ m!QString[(]"([^"]+)"!)
		{
			push @data, $1;
			push @history_, $1;
		}
	}
	close FD;
}

package Namespace;

sub new($$)
{
	my ($z, $t) = @_;
        return bless {title => lc($t)}, $z;
}

sub macro($$)
{
	my $self = shift;
	return "__@{[uc($self->{title})]}_@{[uc($_[0])]}_H__";
}

sub qualify($$)
{
	my $self = shift;
	return "@{[ucfirst($self->{title})]}::Xml::$_[0]";
}

sub cpp($$)
{
	my $self = shift;
	return "$self->{title}_$_[0].cpp";
}

sub header($$)
{
	my $self = shift;
	return "$self->{title}_$_[0].h";
}

sub envelope($@)
{
	my $self = shift;
	my @output;
	push @output, "namespace @{[ucfirst($self->{title})]}";
	push @output, '{';
	push @output, 'namespace Xml';
	push @output, '{', @_, '';
	push @output, '} // namespace Xml';
	push @output, "} // namespace @{[ucfirst($self->{title})]}";
	return @output;
}

package Factory;
use Data::Dumper;
use Scalar::Util 'refaddr';
our %registry;
our @datas;
our @enums;
our @types;
our $namespace;
our @dependencies;

sub build_
{
	my $r = [];
	my ($y, $s, $z, @a) = @_;
	if (exists $registry{$y})
	{
		$r = $registry{$y};
		$y = $y.scalar(@$r);
	}
	else
	{
		$registry{$y} = $r;
	}
	my $output;
	if (ref($z) eq 'CODE')
	{
		$output = $z->($y);
	}
	else
	{
		$output = $z->new($y, @a);
	}
	$output->setNamespace($namespace);
	push @$r, $output;
	push @$s, $output;
	return $output;
}

sub buildComplexType($$$$)
{
	my ($z, $a, $b, $c) = @_;

	return build_(main::getCppNameString($a), \@types, 'Ng::ComplexType', $b, $c);
}

sub beginVariant($$;@)
{
	my ($z, $n, @a) = @_;

	return build_('V'.main::getCppNameString($n), \@types, 'Ng::Variant', @a);
}

sub endComplexVariant($$)
{
	my ($z, $v) = @_;
	@types = grep {refaddr($_) != refaddr($v)} @types;
	push @types, $v;
}

sub endDataVariant($$)
{
	my ($z, $v) = @_;
	my $p = $v->getPattern();
	@types = grep {refaddr($_) != refaddr($v)} @types;
	foreach my $d (@datas)
	{
		next unless $d->can('getPattern');
		if ($d->getPattern() eq $p)
		{
			my $a = Ng::Variant::Typedef->alias($d, $v);
			$a->setNamespace($namespace);
			push @datas, $a;
			return;
		}
	}
	push @datas, $v;
}

sub buildEnum($$$)
{
	my ($z, $n, $v) = @_;

	return build_('E'.main::getCppNameString($n), \@enums, 'Ng::Enum', $v);
}

sub buildData($$;$$$)
{
	my ($z, $n, $a, $p, $e) = @_;
	return Ng::Pod->new($n) unless ref($p) eq 'HASH';

	my $k = 'P'.ucfirst($a);
	if (exists $registry{$k})
	{
		my $t = Ng::Pod->new($n, $k, $p, $e);
#print STDERR ucfirst($k), ' -> ', $registry{$k}[0]->{valueType}->getValueType(), ', ', $t->{valueType}->getValueType(), $/;
		my @m = grep {$_->{valueType}->getValueType() eq $t->{valueType}->getValueType()}
				@{$registry{$k}};
		return shift @m if @m;
	}
	return build_($k, \@datas, sub {return Ng::Pod->new($n, $_[0], $p, $e); });
}

sub produceEnums($)
{
	my $h;
	open $h, ">@{[$namespace->header('enum')]}" or die "Cannot open file: $!";
	print $h <<HEADER;
$Strings::license
#ifndef @{[$namespace->macro('enum')]}
#define @{[$namespace->macro('enum')]}
#include "enum.h"

namespace Libvirt
{
HEADER
	map {print $h $_, $/} $namespace->envelope(map {$_->getDefinition(), ''} @enums);
	print $h <<FOOTER;
} // namespace Libvirt

#endif // @{[$namespace->macro('enum')]}
FOOTER
	close $h;
	open $h, ">@{[$namespace->cpp('enum')]}" or die "Cannot open file: $!";

	print $h <<HEADER;
$Strings::license
#include "@{[$namespace->header('enum')]}"
#include <boost/assign/list_of.hpp>

namespace ba = boost::assign;
namespace Libvirt
{
HEADER
	foreach my $x (@enums)
	{
		map {print $h $_, $/} $x->getTraits();
		print $h $/;
	}
	print $h <<'FOOTER';
} // namespace Libvirt
FOOTER
	close $h;
}

sub produceDatas($)
{
	my $h;
	open $h, ">@{[$namespace->header('data')]}" or die "Cannot open file: $!";
	print $h <<HEADER;
$Strings::license
#ifndef @{[$namespace->macro('data')]}
#define @{[$namespace->macro('data')]}
#include <QString>
#include <QDateTime>
#include "base.h"
#include "patterns.h"
#include "@{[$namespace->header('enum')]}"

namespace Libvirt
{
HEADER
	foreach my $x (@datas)
	{
		my @b = $x->getDeclaration() or next;
		print $h <<HEADER;
///////////////////////////////////////////////////////////////////////////////
// struct @{[$x->getName()]}

HEADER
		map {print $h $_, $/} @b;
		print $h $/;
	}
	print $h <<FOOTER;
} // namespace Libvirt

#endif // @{[$namespace->macro('data')]}
FOOTER
	close $h;

	open $h, ">@{[$namespace->cpp('data')]}" or die "Cannot open file: $!";
	print $h <<HEADER;
$Strings::license
#include "@{[$namespace->header('data')]}"
#include <QRegExp>

namespace Libvirt
{
HEADER
	foreach my $x (@datas)
	{
		my @b = $x->getDefinition() or next;
		print $h <<HEADER;
///////////////////////////////////////////////////////////////////////////////
// struct @{[$x->getName()]}

HEADER
		map {print $h $_, $/} @b;
	}
	print $h <<'FOOTER';
} // namespace Libvirt
FOOTER
	close $h;
}

sub produceTypes($)
{
	my $h;
	open $h, ">@{[$namespace->cpp('type')]}" or die "Cannot open file: $!";
	print $h <<HEADER;
$Strings::license
#include "@{[$namespace->header('type')]}"

namespace Libvirt
{
HEADER
	foreach my $x (@types)
	{
		my @b = $x->getDefinition() or next;
		print $h <<HEADER;
///////////////////////////////////////////////////////////////////////////////
// struct @{[$x->getName()]}

HEADER
		map {print $h $_, $/} @b;
		print $h $/;
	}
	print $h <<'FOOTER';
} // namespace Libvirt
FOOTER
	close $h;
	open $h, ">@{[$namespace->header('type')]}" or die "Cannot open file: $!";
	my @i = ('base.h', $namespace->header('data'), $namespace->header('enum'), 'patterns.h');
	push @i, map {$_->getNamespace()->header('type')} @dependencies;
	print $h <<HEADER;
$Strings::license
#ifndef @{[$namespace->macro('type')]}
#define @{[$namespace->macro('type')]}
@{[join "\n", map {qq/#include "$_"/} @i]}
#include <boost/any.hpp>

namespace Libvirt
{
HEADER
	foreach my $x (grep {$_->can('getForward')} @types)
	{
		map {print $h $_, $/} $x->getForward();
	}
	print $h $/;
	foreach my $x (@types)
	{
		print $h <<HEADER;
///////////////////////////////////////////////////////////////////////////////
// struct @{[$x->getName()]}

HEADER
		my @b = $x->getDeclaration() or next;
		map {print $h $_, $/} @b;
		print $h $/;
	}
	foreach my $x (@types)
	{
		my @b = $x->getTraits() or next;
		print $h <<HEADER;
///////////////////////////////////////////////////////////////////////////////
// struct @{[$x->getName()]} traits

HEADER
		map {print $h $_, $/} @b;
		print $h $/;
	}
	print $h <<FOOTER;
} // namespace Libvirt

#endif // @{[$namespace->macro('type')]}
FOOTER
	close $h;
}

sub produceTexts($)
{
	open my $h, '>text.cpp' or die "Cannot open file: $!";
	print $h <<HEADER;
$Strings::license
#include <QString>

HEADER
	map {print $h $_, $/} Strings->getDefinition();
	close $h;
}

sub isForwardType($$$)
{
	my ($z, $t, $q) = @_;
	my @x = grep({$t->getName() eq $types[$_]->getName()} 0 .. $#types) or return 0;
	return scalar(map {$types[$_]->setForwardDeclaration(); $_}
		grep {$q->getName() eq $types[$_]->getName()} $x[0] .. $#types);
}

sub setup($$)
{
	%registry = ();
	@datas = ();
	@enums = ();
	@types = ();
	@dependencies = ();
	$namespace = Namespace->new($_[1]);
}

sub addDependency($$)
{
	push @dependencies, $_[1];
}

package PlainDecorator;

sub getValueType($$;$)
{
	my ($clazz, $type, $namespace) = @_;
	return $type->getValueType($namespace) if ref($type);

	return $namespace ? $namespace->qualify($type) : $type;
}

sub getParamType($$;$)
{
	my ($clazz, $type, $namespace) = @_;
	return $clazz->getValueType($type, $namespace);
}

sub getResultType($$;$)
{
	my ($clazz, $type, $namespace) = @_;
	return $clazz->getValueType($type, $namespace);
}

sub getDefaultInitializer($$)
{
	my ($clazz, $member) = @_;
	return "@{[$member->{variable}]}()";
}

sub getParser($)
{
	my @output;
	push @output, "bool output = false;";
	push @output, "dst_ = src_.$_[0](&output);";
	push @output, "return output;";

	return @output;
}

sub getGenerator()
{
	return ("return QString::number(src_);");
}

package ComplexDecorator;

sub getValueType($$;$)
{
	my ($clazz, $type, $namespace) = @_;
	return PlainDecorator->getValueType($type, $namespace);
}

sub getDecorated($$)
{
	my ($clazz, $name) = @_;
	return "const ${name}&";
}

sub getParamType($$;$)
{
	my ($clazz, $type, $namespace) = @_;
	return $clazz->getDecorated($clazz->getValueType($type, $namespace));
}

sub getResultType($$;$)
{
	my ($clazz, $type, $namespace) = @_;
	return $clazz->getDecorated($clazz->getValueType($type, $namespace));
}

sub getDefaultInitializer($$)
{
	return undef;
}

package OptionalDecorator;

sub getBoostType($$;$)
{
	my ($clazz, $type, $namespace) = @_;
	return "boost::optional<@{[PlainDecorator->getValueType($type, $namespace)]} >";
}

sub getBoolean($)
{
	my ($type, $v) = @_;
	$v = ComplexDecorator->getValueType($type);
	return 'bool' eq $v ? $v : undef;
}

sub getValueType($$;$)
{
	my ($clazz, $type, $namespace) = @_;
	return $type->getValueType($namespace) if ref($type) && $type->getDecorator() eq __PACKAGE__;

	return getBoolean($type) || $clazz->getBoostType($type, $namespace);
}

sub getParamType($$;$)
{
	my ($clazz, $type, $namespace) = @_;
	return $type->getParamType($namespace) if ref($type) && $type->getDecorator() eq __PACKAGE__;

	return getBoolean($type) || ComplexDecorator->getDecorated($clazz->getBoostType($type, $namespace));
}

sub getResultType($$;$)
{
	my ($clazz, $type, $namespace) = @_;
	return $type->getResultType($namespace) if ref($type) && $type->getDecorator() eq __PACKAGE__;

	return getBoolean($type) || ComplexDecorator->getDecorated($clazz->getBoostType($type, $namespace));
}

sub getDefaultInitializer($$)
{
	my ($clazz, $member) = @_;
	getBoolean($member->{cppType}) or return undef;

	return "@{[$member->{variable}]}()";
}

package QListDecorator;

sub getDecorated($$)
{
	my ($clazz, $name) = @_;
	return "QList<${name} >";
}

sub getValueType($$;$)
{
	my ($clazz, $type, $namespace) = @_;
	return $clazz->getDecorated(PlainDecorator->getValueType($type, $namespace));
}

sub getParamType($$;$)
{
	my ($clazz, $type, $namespace) = @_;
	return ComplexDecorator->getDecorated($clazz->getValueType($type, $namespace));
}

sub getResultType($$;$)
{
	my ($clazz, $type, $namespace) = @_;
	return ComplexDecorator->getDecorated($clazz->getValueType($type, $namespace));
}

sub getDefaultInitializer($$)
{
	return undef;
}

package IncompleteTypeDecorator;
use Data::Dumper;

sub getReader($$$)
{
	my ($clazz, $type, $any) = @_;
	my @output;
	push @output, "if (${any}.empty())";
	push @output, "\treturn NULL;";
	push @output, '';
	push @output, "return boost::any_cast<@{[PlainDecorator->getValueType($type)]} >(&${any});";
	return @output;
}

sub getWritter($$$)
{
	my ($clazz, $any, $value) = @_;
	return ("${any} = ${value};");
}

sub getValueType($$;)
{
	my ($clazz, $type) = @_;
	return "boost::any";
}

sub getParamType($$;$)
{
	my ($clazz, $type, $namespace) = @_;
	return $type->getParamType($namespace) if ref($type);

	my $x = $namespace ? $namespace->qualify($type) : $type;
	return ComplexDecorator->getDecorated($x);
}

sub getResultType($$;$)
{
	my ($clazz, $type, $namespace) = @_;
	my $x = ref($type) ? $type->getValueType($namespace) : $namespace ? $namespace->qualify($type) : $type;
	return "const ${x}*";
}

sub getDefaultInitializer($$)
{
	my ($clazz, $member) = @_;
	return undef;
}

package ValueType;

sub new($$$)
{
	return bless {type => $_[1], decorator => $_[2]}, $_[0];
}

sub getName($)
{
	my $self = shift;
	return $self->{type}->getValueType() if ref($self->{type});

	return $self->{type};
}

sub getDecorator($)
{
	my $self = shift;
	return $self->{decorator};
}

sub getValueType($;$)
{
	my ($self, $namspace) = @_;
	return $self->getDecorator()->getValueType($self->{type}, $namspace);
}

sub getParamType($;$)
{
	my ($self, $namspace) = @_;
	return $self->getDecorator()->getParamType($self->{type}, $namspace);
}

sub getResultType($;$)
{
	my ($self, $namspace) = @_;
	return $self->getDecorator()->getResultType($self->{type}, $namspace);
}

package Ng::DataType;

sub new($$;$)
{
	my ($z, $n, $c) = @_;
        return bless {cppType => $c || $n, name => $n}, $z; 
}

sub getName($)
{
	my $self = shift;
	return $self->{name};
}

sub getQualifiedName($)
{
	my $self = shift;
	my $x = $self->getName();
	return $x unless $self->{cppNamespace};

	return $self->getNamespace()->qualify($x);
}

sub getCppType($)
{
	my $self = shift;
	return $self->{cppType};
}

sub getNamespace($)
{
	my $self = shift;
	return $self->{cppNamespace};
}

sub setNamespace($$)
{
	my $self = shift;
	$self->{cppNamespace} = shift;
}

package Ng::Constant;

sub new($$)
{
	my ($z, $v) = @_;
        return bless {value => $v}, $z;
}

sub getName($)
{
	my $self = shift;
	return 'mpl::int_<'.$self->{value}.'>';
}

package Ng::Pod;
use Data::Dumper;
our @ISA = qw(Ng::DataType);

our %builtin = (string => ValueType->new('QString', 'ComplexDecorator'),
		'token' => ValueType->new('QString', 'ComplexDecorator'),
		'' => ValueType->new('QString', 'ComplexDecorator'),
		'NCName' => ValueType->new('QString', 'ComplexDecorator'),
		'dateTime' => ValueType->new('QDateTime', 'ComplexDecorator'),
		'boolean' => ValueType->new('bool', 'PlainDecorator'),
		'int' => ValueType->new('qint32', 'PlainDecorator'),
		'integer' => ValueType->new('qint32', 'PlainDecorator'),
		'unsignedInt' => ValueType->new('quint32', 'PlainDecorator'),
		'positiveInteger' => ValueType->new('quint32', 'PlainDecorator'),
		'long' => ValueType->new('long', 'PlainDecorator'),
		'short' => ValueType->new('qint16', 'PlainDecorator'),
		'double' => ValueType->new('double', 'PlainDecorator'),
		'unsignedShort' => ValueType->new('quint16', 'PlainDecorator'),
		'unsignedLong' => ValueType->new('ulong', 'PlainDecorator'));

our %marshals = ('dateTime' => {parse => sub { return getParseDateTime(); }, generate => sub { return ("return src_.toString();"); }},
		'short' => {parse => sub { return PlainDecorator::getParser('toShort'); }, generate => sub { return PlainDecorator::getGenerator(); }},
		'unsignedShort' => {parse => sub { return PlainDecorator::getParser('toUShort'); }, generate => sub { return PlainDecorator::getGenerator(); }},
		'int' => {parse => sub { return PlainDecorator::getParser('toInt'); }, generate => sub { return PlainDecorator::getGenerator(); }},
		'integer' => {parse => sub { return PlainDecorator::getParser('toInt'); }, generate => sub { return PlainDecorator::getGenerator(); }},
		'unsignedInt' => {parse => sub { return PlainDecorator::getParser('toUInt'); }, generate => sub { return PlainDecorator::getGenerator(); }},
		'positiveInteger' => {parse => sub { return PlainDecorator::getParser('toUInt'); }, generate => sub { return PlainDecorator::getGenerator(); }},
		'long' => {parse => sub { return PlainDecorator::getParser('toLong'); }, generate => sub { return PlainDecorator::getGenerator(); }},
		'double' => {parse => sub { return PlainDecorator::getParser('toDouble'); }, generate => sub { return PlainDecorator::getGenerator(); }},
		'unsignedLong' => {parse => sub { return PlainDecorator::getParser('toULong'); }, generate => sub { return PlainDecorator::getGenerator(); }});

sub getParseDateTime($)
{
	my @output;
	push @output, "dst_ = QDateTime::fromString(src_);";
	push @output, "return !dst_.isNull();";

	return @output;
}

## name, alias, params{}, excepts[]
sub new($$;$$$)
{
	my ($z, $n, $a, $p, $e) = @_;
	my $x = $builtin{$n};
	my $output;
	if ('HASH' eq ref($p))
	{
print '"', $n, '"', $/ unless $x;
		$output = $z->SUPER::new($a, ValueType->new("${a}::value_type", $x->getDecorator()));
		$output->{params} = $p;
		$output->{marshals} = $marshals{$n} if exists $marshals{$n};
		$output->{excepts} = $e;
		$output->{valueType} = $x;
	}
	else
	{
		$output = $z->SUPER::new($x->getValueType(), $x);
	}
        return bless $output, $z; 
}

sub validator_($)
{
	my $self = shift;
	if (exists($self->{valueType}) && not exists $self->{validator})
	{
		my @v;
		if (exists $self->{marshals})
		{
			if (exists $self->{params}{minInclusive})
			{
				push @v, "\tif ($self->{params}{minInclusive} > value_)";
				push @v, "\t\treturn false;";
				push @v, '';
			}
			if (exists $self->{params}{maxInclusive})
			{
				push @v, "\tif ($self->{params}{maxInclusive} < value_)";
				push @v, "\t\treturn false;";
				push @v, '';
			}
		}
		else
		{
			if (exists $self->{params}{maxLength})
			{
				push @v, "\tif ($self->{params}{maxLength} < value_.length())";
				push @v, "\t\treturn false;";
				push @v, '';
			}
			foreach my $x (@{$self->{excepts}})
			{
				$x =~ s/\n/\\n/g;
				push @v, qq(\tif ("$x" == value_));
				push @v, "\t\treturn false;";
				push @v, '';
			}
			if (exists $self->{params}{pattern})
			{
				my $p = $self->{params}{pattern};
				$p =~ s/(?<!\\)(x[0-9a-fA-F]{2})/\\$1/g;
				$p =~ s/\\/\\\\/g;
				$p =~ s/"/\\"/g;
				$p =~ s/\n/\\n/g;
				push @v, qq(\tQRegExp q("$p"););
				push @v, "\tif (!q.exactMatch(value_))";
				push @v, "\t\treturn false;";
				push @v, '';
			}
		}
		if (@v)
		{
			my (@f, @c);
			my $q = $self->getQualifiedName();
			push @c, 'template<>';
			push @c, "struct Validatable<$q>: mpl::true_";
			push @c, '{';
			push @c, "\tstatic bool validate(@{[$self->getCppType()->getParamType($self->getNamespace())]} value_);";
			push @c, '';
			push @c, '};';

			push @f, "bool Validatable<$q>::validate(@{[$self->getCppType()->getParamType($self->getNamespace())]} value_)";
			push @f, '{', @v, "\treturn true;", '}', '';
			$self->{validator} = {definition => \@f, declaration => \@c};
		}
	}
	return exists $self->{validator};
}

sub getDeclaration($)
{
	my $self = shift;
	my @output;
	if (exists $self->{valueType})
	{
		my @x;
		push @x, 'struct '.$self->getName();
		push @x, '{';
		push @x, "\ttypedef @{[$self->{valueType}->getValueType()]} value_type;";
		push @x, '};';
		push @output, $self->getNamespace()->envelope(@x), '';
	}
	if (exists $self->{marshals})
	{
		my $q = $self->getQualifiedName();
		push @output, 'template<>';
		push @output, "struct Traits<$q>";
		push @output, '{';
		push @output, "\tstatic bool parse(const QString& src_, @{[$self->getCppType()->getValueType($self->getNamespace())]}& dst_);";
		push @output, '';
		push @output, "\tstatic QString generate(@{[$self->getCppType()->getParamType($self->getNamespace())]} src_);";
		push @output, '';
		push @output, '};';
		push @output, '';
	}
	if ($self->validator_())
	{
		push @output, @{$self->{validator}{declaration}};
	}
	return @output;
}

sub getDefinition($$)
{
	my $self = shift;
	my @output;
	if (exists $self->{marshals})
	{
		my $q = $self->getQualifiedName();
		push @output, "bool Traits<$q>::parse(const QString& src_, @{[$self->getCppType()->getValueType($self->getNamespace())]}& dst_)";
		push @output, "{";
		if (exists $self->{params}{pattern})
		{
			push @output, qq(\tQRegExp q("$self->{params}{pattern}"););
			push @output, "\tif (!q.exactMatch(src_))";
			push @output, "\t\treturn false;";
		}
		push @output, map {"\t$_"} $self->{marshals}{parse}->();
		push @output, "}";
		push @output, '';
		push @output, "QString Traits<$q>::generate(@{[$self->getCppType()->getParamType($self->getNamespace())]} src_)";
		push @output, "{";
		push @output, map {"\t$_"} $self->{marshals}{generate}->();
		push @output, "}";
		push @output, '';
	}
	if ($self->validator_())
	{
		push @output, @{$self->{validator}{definition}};
	}
	return @output;
}

package Ng::Enum;
use Data::Dumper;
our @ISA = qw(Ng::DataType);

sub new($$$)
{
	my ($z, $n, $v) = @_;
	die "an ARRAY reference is required!" unless ref($v) eq 'ARRAY';

	my $output = $z->SUPER::new($n, ValueType->new($n, 'PlainDecorator'));
	my @g;
	foreach my $x (@$v)
	{
		my $e = $n.main::getCppNameString($x);
		push @g, {enumerator => $e, text => Strings->lookup($x)};
	}
	$output->{values} = \@g;
	return bless $output, $z;
}

sub getTraits($)
{
	my $self = shift;
	my @output;
	my $q = $self->getQualifiedName();
	push @output, 'template<>';
	push @output, "Enum<$q>::data_type Enum<$q>::getData()";
	push @output, '{';
	push @output, "\treturn ba::list_of<data_type::relation>";
	push @output, map {"\t\t\t(data_type::value_type(".$self->getNamespace()->qualify($_->{enumerator}).', "'.$_->{text}.'"))'} @{$self->{values}};
	$output[$#output] .= ';';
	push @output, '}';
	return @output;
}

sub getDefinition($)
{
	my $self = shift;
	my @output;
	my @v = map {$_->{enumerator}} @{$self->{values}};
	map {$v[$_] .= ','} 0..$#v - 1;
	push @output, 'enum '.$self->getName();
	push @output, '{';
	push @output, map({"\t$_"} @v);
	push @output, '};';
	return @output;
}

package Ng::Alias;

sub new($$$)
{
	my ($z, $t, $n) = @_;
	return bless {target => $t, name => $n}, ref($z) || $z;
}

sub getDeclaration($)
{
	my $self = shift;
	return join(' ', ('typedef', $self->{target}, $self->{name})).';';
}

package Ng::Variant::Typedef;
our @ISA = qw(Ng::DataType);

sub alias($$$)
{
	my ($z, $t, $a) = @_;
	my $r = $a->getCppType()->getValueType();
	my $l = $t->getCppType()->getValueType();
	my $output = $z->SUPER::new($r, $a->getCppType());
	$output->{impl} = Ng::Alias->new("${l}Impl", "${r}Impl");
	$output->{value} = Ng::Alias->new($l, $r);
	return bless $output, ref($z) || $z;
}

sub body($$)
{
	my ($z, $v) = @_;
	my $n = $v->getCppType()->getValueType();
	my $output = $z->SUPER::new($n, $v->getCppType());
	my $i = "${n}Impl";
	$output->{impl} = Ng::Alias->new($v->getPattern(), $i);
	$output->{value} = Ng::Alias->new("${i}::value_type", $n);
	if ($v->getForward())
	{
		$output->{bin} = Ng::Forward->new($v->getCppType());
	}
	return bless $output, ref($z) || $z;
}

sub getDeclaration($)
{
	my $self = shift;
	my @output;
	push @output, $self->{impl}->getDeclaration();
	push @output, $self->{value}->getDeclaration();
	push @output, $self->{bin}->getDefinition() if exists $self->{bin};
	my $n = $self->getNamespace();
	$n and @output = $n->envelope(@output);
	return @output;
}

sub getDefinition($)
{
	my $self = shift;
	return ();
}

package Ng::Variant::Traits;

sub new($)
{
	my $z = shift;
	return bless {}, ref($z) || $z;
}

sub getDeclaration($$)
{
	my ($self, $variant) = @_;
	my @output;
	my $n = $variant->getNamespace();
	my $q = $variant->getCppType()->getValueType($n);
	push @output, 'template<>';
	push @output, "struct Traits<$q>";
	push @output, '{';
	push @output, "\tstatic bool parse(const QString& src_, ${q}& dst_);";
	push @output, '';
	push @output, "\tstatic QString generate(@{[$variant->getCppType()->getParamType($n)]} src_);";
	push @output, '';
	push @output, '};';
	return @output;
}

sub getDefinition($$)
{
	my ($self, $variant) = @_;
	my ($i, @output) = 0;
	my $n = $variant->getNamespace();
	my $q = $variant->getCppType()->getValueType($n);
	push @output, "bool Traits<$q>::parse(const QString& src_, ${q}& dst_)";
	push @output, "{";
	push @output, "\tint x;";
	foreach my $a ($variant->getAlternatives())
	{
		next unless $a->can('getName');
		push @output, "\tmpl::at_c<${q}::types, $i>::type a${i};";
		push @output, "\tx = Marshal<@{[$a->getQualifiedName()]}>::setString(src_, a${i});";
		push @output, "\tif (0 < x)";
		push @output, "\t{";
		push @output, "\t\tdst_ = a${i};";
		push @output, "\t\treturn true;";
		push @output, "\t}";
		++$i;
	}
	push @output, '';
	push @output, "\treturn false;";
	push @output, '}';
	push @output, '';
	$i = 0;
	push @output, "QString Traits<$q>::generate(@{[$variant->getCppType()->getParamType($n)]} src_)";
	push @output, "{";
	push @output, "\tswitch (src_.which())";
	push @output, "\t{";
	foreach my $a ($variant->getAlternatives())
	{
		next unless $a->can('getName');
		push @output, "\tcase ${i}:";
		push @output, "\t\treturn Marshal<@{[$a->getQualifiedName()]}>::getString(boost::get<mpl::at_c<${q}::types, ${i}>::type>(src_));";
		++$i;
	}
	push @output, "\t}";
	push @output, "\treturn QString();";
	push @output, "}";
	push @output, '';
	return @output;
}

package Ng::Variant;
our @ISA = qw(Ng::DataType);

sub new($$;@)
{
	my ($z, $x, @a) = @_;
	my $output = $z->SUPER::new($x, ValueType->new($x, 'ComplexDecorator'));
	$output->{alternatives} = [@a];
        return bless $output, $z, 
}

sub setTraits($$)
{
	my ($self, $traits) = @_;
	$self->{traits} = $traits;
}

sub addAlternative($$)
{
	my ($self, $a) = @_;
	push @{$self->{alternatives}}, $a;
}

sub getAlternatives($)
{
	my $self = shift;
	return @{$self->{alternatives}};
}

sub getPattern($)
{
	my $self = shift;
	my (@v, @output);
	foreach my $x ($self->getAlternatives())
	{
		if ($x->can('getMarshal'))
		{
			push @v, $x->getMarshal()->{cppType};
		}
		elsif ($x->can('getName'))
		{
			push @v, $x->getName();
		}
	}
	return "Choice<mpl::vector<@{[join ', ', @v]} > >";
}

sub setForwardDeclaration($)
{
	my $self = shift;
	$self->{forward} = Ng::Forward->new($self->getCppType());
	$self->{forward}->setNamespace($self->getNamespace());
}

sub getForward($)
{
	my $self = shift;
	return () unless $self->{forward};
	return ($self->{forward}->getDeclaration());
}

sub getTraits($)
{
	my $self = shift;
	return ();
}

sub getDefinition($)
{
	my $self = shift;
	return $self->{traits}->getDefinition($self) if $self->{traits};

	return ();
}

sub getDeclaration($)
{
	my $self = shift;
	my $n = $self->getNamespace();
	my @output;
	if ($n)
	{
		my $b = Ng::Variant::Typedef->body($self);
		$b->setNamespace($n);
		push @output, $b->getDeclaration();
	}
	else
	{
		push @output, Ng::Variant::Typedef->body($self)->getDeclaration();
	}
	if ($self->{traits})
	{
		push @output, '';
		push @output, $self->{traits}->getDeclaration($self);
	}
	return @output;
}

package Ng::ComplexType;
use Data::Dumper;
our @ISA = qw(Ng::DataType);

sub new($$$$)
{
	my ($z, $n, $m, $k) = @_;
	my $output = $z->SUPER::new($n, ValueType->new($n, 'ComplexDecorator'));
	$output->{knot} = $k;
	$output->{members} = $m;
        return bless $output, $z;
}

sub members_($)
{
	my $self = shift;
	return @{$self->{pmembers}} if exists $self->{pmembers};

	my (@g, %n);
	foreach my $x (@{$self->{members}})
	{
		my $m = $x->{member}->getMember() or next;
		my %y = %$x;
		$y{member} = $m;
		my $z = $m->{name};
		for (my $i = 2; exists $n{$z}; ++$i)
		{
			$z = "$m->{name}$i";
		}
		$n{$z} = 1;
		$m->{name} = $z;
		my $t = $m->{cppType};
		my $v = 'm_'. lcfirst($z);
		if (Factory->isForwardType($self, $t))
		{
			$m->{forward} = Ng::Forward->new($t);
			$m->{forward}->setNamespace($self->getNamespace());
			my $f = $m->{forward}->getCppType();
			$m->{cppType} = ValueType->new($f, 'IncompleteTypeDecorator');
		}
		$m->{variable} = $v;
		push @g, \%y;
	}
	$self->{pmembers} = [@g];
	return $self->members_();
}

sub initializers_($)
{
	my $self = shift;
	return @{$self->{pinitializers}} if exists $self->{pinitializers};

	my @g;
	foreach my $x ($self->members_())
	{
		my $m = $x->{member};
		my $t = $m->{cppType};
		my $i = $t->getDecorator()->getDefaultInitializer($m) or next;
		push @g, $i;
	}
	$self->{pinitializers} = [@g];
	return $self->initializers_();
}

sub setSerializer($$)
{
	my ($self, $value) = @_;
	$self->{serializer} = $value;
}

sub getDeclaration($)
{
	my $self = shift;
	my (@output, $n);
	push @output, 'struct '. $self->getName();
	push @output, "{";
	if ($self->members_())
	{
		if ($self->initializers_())
		{
			push @output, "\t".$self->getName().'();';
			push @output, '';
		}
		foreach my $m (map {$_->{member}} $self->members_())
		{
			my $n = $m->{name};
			my $t = $m->{cppType};
			if ($m->{forward})
			{
				push @output, "\t@{[$t->getResultType()]} get$n() const;";
				push @output, "\tvoid set$n(@{[$t->getParamType()]} value_);";
			}
			else
			{
				push @output, "\t@{[$t->getResultType()]} get$n() const";
				push @output, "\t{";
				push @output, "\t\treturn $m->{variable};";
				push @output, "\t}";
				push @output, "\tvoid set$n(@{[$t->getParamType()]} value_)";
				push @output, "\t{";
				push @output, "\t\t$m->{variable} = value_;";
				push @output, "\t}";
			}
		}
		if ($self->{serializer})
		{
			push @output, "\tbool load(const QDomElement& );";
			push @output, "\tbool save(QDomElement& ) const;";
			if ($self->{serializer}->can('getTopSaviour'))
			{
				push @output, "\tbool save(QDomDocument& ) const;";
			}
		}
		push @output, '';
		push @output, 'private:';
		foreach my $m (map {$_->{member}} $self->members_())
		{
			my $n = $m->{name};
			push @output, "\t@{[$m->{cppType}->getValueType()]} $m->{variable};";
		}
	}
	push @output, "};";
	$n = $self->getNamespace() and @output = $n->envelope(@output);
	return @output;
}

sub getTraits($)
{
	my $self = shift;
	my @output;
	push @output, 'template<>';
	push @output, 'struct Traits<'.$self->getQualifiedName().'>';
	push @output, '{';
	push @output, "\ttypedef ".$self->{knot}->getMarshal($self->getNamespace())->{cppType}.' marshal_type;';
	push @output, '';
	push @output, "\tstatic int parse(".$self->getQualifiedName().'& , QStack<QDomElement>& );';
	push @output, "\tstatic int generate(const ".$self->getQualifiedName().'& , QDomElement& );';
	push @output, "};";
	return @output;
}

sub getDefinition($)
{
	my $self = shift;
	my (@b, @w);
	if ($self->initializers_())
	{
		push @b, "$self->{name}::$self->{name}(): ". join ', ', $self->initializers_();
		push @b, '{';
		push @b, '}';
		push @b, '';
		foreach my $m (grep {$_->{forward}} map {$_->{member}} $self->members_())
		{
			my $n = $m->{name};
			my $t = $m->{cppType};

			push @b, "@{[$t->getResultType()]} $self->{name}::get$n() const";
			push @b, "{";
			push @b, map {"\t$_"} IncompleteTypeDecorator->getReader($m->{forward}->getCppType(), $m->{variable});
			push @b, "}";
			push @b, '';
			push @b, "void $self->{name}::set$n(@{[$t->getParamType()]} value_)";
			push @b, "{";
			push @b, map {"\t$_"} IncompleteTypeDecorator->getWritter($m->{variable}, 'value_');
			push @b, '}', '';
		}
	}
	my $q = $self->getQualifiedName();
	if ($self->members_())
	{
		if ($self->{serializer})
		{
			push @b, "bool $self->{name}::load(const QDomElement& src_)";
			push @b, "{";
			push @b, map {"\t$_"} $self->{serializer}->getLoader("src_");
			push @b, "}";
			push @b, '';
			push @b, "bool $self->{name}::save(QDomElement& dst_) const";
			push @b, "{";
			push @b, map {"\t$_"} $self->{serializer}->getSaviour("dst_");
			push @b, "}";
			push @b, '';
			if ($self->{serializer}->can('getTopSaviour'))
			{
				push @b, "bool $self->{name}::save(QDomDocument& dst_) const";
				push @b, "{";
				push @b, map {"\t$_"} $self->{serializer}->getTopSaviour("dst_");
				push @b, '}', '';
			}
		}
		push @w, "int Traits<${q}>::parse(${q}& dst_, QStack<QDomElement>& stack_)";
		push @w, "{";
		push @w, "\tmarshal_type m;";
		push @w, "\tint output = m.consume(stack_);";
		push @w, "\tif (0 <= output)";
		push @w, "\t{";
		foreach my $x ($self->members_())
		{
			my $m = $x->{member};
			my $g = join '.', map {"get<$_>()"} @{$x->{path}};
			my $r = "m.$g.getValue()";
			if ($m->{forward})
			{
				push @w, map {"\t\t$_"} $m->{forward}->getRecipient('b', $r);
				$r = 'b';
			}
			push @w, "\t\tdst_.set$m->{name}(${r});";
		}
		push @w, "\t}";
		push @w, "\treturn output;";
		push @w, "}";
		push @w, '';
		push @w, "int Traits<${q}>::generate(const ${q}& src_, QDomElement& dst_)";
		push @w, "{";
		push @w, "\tmarshal_type m;";
		foreach my $x ($self->members_())
		{
			my $m = $x->{member};
			my $g = join '.', map {"get<$_>()"} @{$x->{path}};
			my $r = "src_.get$m->{name}()";
			if ($m->{forward})
			{
				push @w, map {"\t$_"} $m->{forward}->getDonor('d', $r);
				push @w, "\tif (NULL == d)";
				push @w, "\t\treturn -1;";
				$r = '*d';
			}
			push @w, "\tif (0 > Details::Marshal::assign($r, m.$g))";
			push @w, "\t\treturn -1;";
		}
		push @w, '';
		push @w, "\treturn m.produce(dst_);";
		push @w, "}";
	}
	else
	{
		push @w, "int Traits<${q}>::parse(${q}& , QStack<QDomElement>& stack_)";
		push @w, "{";
		push @w, "\tmarshal_type m;";
		push @w, "\treturn m.consume(stack_);";
		push @w, "}";
		push @w, '';
		push @w, "int Traits<${q}>::generate(const ${q}& , QDomElement& dst_)";
		push @w, "{";
		push @w, "\tmarshal_type m;";
		push @w, "\treturn m.produce(dst_);";
		push @w, "}";
	}
	my $n = $self->getNamespace();
	return (@b, @w) unless @b && $n;

	return $n->envelope(@b), '', @w;
}

package Ng::Forward;
our @ISA = qw(Ng::DataType);

sub new($$)
{
	my ($z, $t) = @_;
	my $n = $t->getValueType();
	my $output = $z->SUPER::new("${n}Bin", ValueType->new("${n}Bin", 'ComplexDecorator'));
	$output->{value} = $t;
	return bless $output, $z;
}

sub getDeclaration($)
{
	my $self = shift;
	return $self->getNamespace()->envelope("struct @{[$self->getCppType()->getValueType()]};");
}

sub getDefinition($)
{
	my $self = shift;
	my @output;
	push @output, "struct @{[$self->getCppType()->getValueType()]}";
	push @output, "{";
	push @output, "\t@{[$self->{value}->getValueType()]} value;";
	push @output, "};";
	return @output;
}

sub getRecipient($$$)
{
	my ($self, $v, $s) = @_;
	my @output;
	push @output, "@{[$self->getCppType()->getValueType($self->getNamespace())]} $v;";
	push @output, "${v}.value = $s;";
	return @output;
}

sub getDonor($$)
{
	my ($self, $v, $s) = @_;
	my @output;
	push @output, "const @{[$self->{value}->getValueType($self->getNamespace())]}* ${v} = NULL;";
	push @output, "const @{[$self->getCppType()->getValueType($self->getNamespace())]}* v = ${s};";
	push @output, "if (NULL != v)";
	push @output, "\t${v} = &v->value;";
	return @output;
}

package Ng::Ref;
sub new($$)
{
        return bless {target => Strings->reverse($_[1])}, ref($_[0]) || $_[0];
}

sub getTarget($)
{
	my $self = shift;
	return $main::defines{Strings->lookup($self->{target})};
}

sub getDataType($)
{
	my $self = shift;
	return $self->getTarget()->getDataType();
}

sub expand($)
{
	my $self = shift;
	my @output = @{$self->getTarget()->{children}};
	for (my $i = 0; $i < @output; ++$i)
	{
		my $x = $output[$i];
		next unless $x->can('getTarget');

		splice @output, $i, 1, $x->expand();
		$i = -1;
	}
	return @output;
}

package Ng::Optional;
use Data::Dumper;
use UNIVERSAL qw(isa);
sub new($$)
{
        return bless $_[1], ref($_[0]) || $_[0];
}

sub generate($)
{
	my $self = shift;
	return if exists $self->{inline};

	my $b = Ng::Block->new(0, $self->{children});
	$b->generate();
	if ($b->getType())
	{
		$self->{inline} = Ng::Fragment->new($b->getType());
	}
	else
	{
		$self->{inline} = $b->getInline();
	}
	unless ($self->{inline})
	{
		print Dumper($b), $/;
		die "An optional without a body is detected!";
	}
#	die "An optional without a body is detected!" unless $self->{inline};
}

sub getMember($)
{
	my ($self, $m) = @_;
	$m = $self->{inline}->getMember();
	my $t = ValueType->new($m->{cppType}, 'OptionalDecorator');
	return {name => $m->{name}, cppType => $t};
}

sub getMarshal($;$)
{
	my ($self, $namespace) = @_;
	my $m = $self->{inline}->getMarshal($namespace);
	return {name => '', cppType => 'Optional<'.$m->{cppType}.' >'};
}

sub getDataType($)
{
	my $self = shift;
	return $self->{inline}->getDataType();
}

package Ng::Fragment;
use Data::Dumper;

sub new($$)
{
	my ($z, $t) = @_;
	$t->setSerializer(Ng::Serializer::Fragment->new($t));
        return bless {type => $t}, ref($z) || $z;
}

sub getTypeName($;$)
{
	my ($self, $namespace) = @_;
	my $n = $self->getDataType()->{name};
	return $namespace ? $namespace->qualify($n) : $n;
}

sub getMember($)
{
	my $self = shift;
	return {name => $self->getTypeName(), cppType => $self->getDataType()->getCppType()};
}

sub getMarshal($;$)
{
	my ($self, $namespace) = @_;
	unless ($self->getTypeName())
	{
		print Dumper([$self]), $/;
		die "Type is undefined!";
	}
	return {name => '', cppType => 'Fragment<'.$self->getTypeName($namespace).' >'};
}

sub getDataType($)
{
	my $self = shift;
	return $self->{type};
}

package Ng::OrMore;
use Data::Dumper;

sub one($$)
{
        my $output = bless $_[1], ref($_[0]) || $_[0];
	$output->{marshal} = 'OneOrMore';
	return $output;
}

sub zero($$)
{
        my $output = bless $_[1], ref($_[0]) || $_[0];
	$output->{marshal} = 'ZeroOrMore';
	return $output;
}

sub generate($)
{
	my $self = shift;
	return if exists $self->{inline};

	my $b = Ng::Block->new(0, $self->{children});
	$b->generate();
	if ($b->getType())
	{
		$self->{inline} = Ng::Fragment->new($b->getType());
	}
	else
	{
		$self->{inline} = $b->getInline();
	}
	die "A list without a body is detected!" unless $self->{inline};
}

sub getDataType($)
{
	my $self = shift;
	return $self->{inline}->getDataType();
}

sub getMember($)
{
	my $self = shift;
	my $m = $self->{inline}->getMember();
	my $t = ValueType->new($m->{cppType}, 'QListDecorator');
	return {name => $m->{name}.'List', cppType => $t};
}

sub getMarshal($;$)
{
	my ($self, $namespace) = @_;
	my $m = $self->{inline}->getMarshal($namespace);
	return {name => '', cppType => $self->{marshal}.'<'.$m->{cppType}.' >'};
}

package Ng::Group;
use Data::Dumper;

sub ordered($$)
{
	my $output = {marshal => 'Ordered', children => $_[1]};
        return bless $output, ref($_[0]) || $_[0];
}

sub unordered($$)
{
	my $output = {marshal => 'Unordered', children => $_[1]};
        return bless $output, ref($_[0]) || $_[0];
}

sub generate($)
{
	my $self = shift;
	if (exists $self->{generated})
	{
		return wantarray() ? @{$self->{generated}} : undef;
	}
	my @a = @{$self->{children}};
	for (my $i = 0; $i < scalar(@a); ++$i)
	{
		my $x = $a[$i];
		unless ($x->can('getTarget'))
		{
			$x->generate();
			next;
		}
		my @e = $x->expand();
		unless (1 == @e && $e[0]->can('flatten'))
		{
			$x->getTarget()->generate();
			next;
		}
		my $g = shift @e;
		if ($g->{marshal} eq $self->{marshal})
		{
			splice @a, $i, 1, @{$g->{children}};
			$i = -1;
		}
		else
		{
			$x->getTarget()->generate();
		}
	}
	$self->{generated} = [@a];
	return wantarray() ? @a : undef;
}

sub getMarshal($;$)
{
	my ($self, $namespace) = @_;
	unless (exists $self->{marshals})
	{
		my @m;
		foreach my $x ($self->generate())
		{
			unless ($x->can('getTarget'))
			{
				if ($x->can('getMarshal'))
				{
					push @m, $x->getMarshal($namespace);
				}
				elsif ($x->can('getDataType'))
				{
					push @m, Ng::Fragment->new($x->getDataType())->getMarshal($namespace);
				}
				else
				{
					print Dumper([$x]), $/;
					die "No marshal!!!111";
				}
				next;
			}
			my ($y, $t) = $x->getTarget();
			if ($y->getInlineGroup())
			{
				$t = $y->getInlineGroup();
			}
			elsif ($y->getInline())
			{
				$t = $y->getInline();
			}
			else
			{
				$t = Ng::Fragment->new($y->getType());
			}
			push @m, $t->getMarshal($namespace);
		}
		$self->{marshals} = [@m];
	}
	my $m = join ', ', map {$_->{cppType}} @{$self->{marshals}};
	return {name => '', cppType => $self->{marshal}.'<mpl::vector<'.$m.' > >'};
}

sub flatten($)
{
	my ($self, $i, @output) = (@_, 0);
	my @x;
	foreach my $c ($self->generate())
	{
		if ($c->can('getTarget'))
		{
			my $d = $c->getTarget();
			$d->generate();
			if ($d->getType())
			{
				push @x, $d;
			}
			elsif ($d->getInlineGroup())
			{
				push @x, $d->getInlineGroup();
			}
			else
			{
				push @x, $d->getBody();
			}
		}
		else
		{
			$c->can('generate') && $c->generate();
			push @x, $c;
		}
	}
	foreach my $c (@x)
	{
		if ($c->can('flatten'))
		{
			push @output, map {unshift @{$_->{path}}, $i; $_} $c->flatten();
		}
		elsif ($c->can('getType'))
		{
			push @output, {member => Ng::Fragment->new($c->getType()), path => [$i]};
		}
		else
		{
			push @output, {member => $c, path => [$i]};
		}
		++$i;
	}
	return @output;
}

package Ng::Attribute;
use Data::Dumper;
sub new($$)
{
        return bless $_[1], ref($_[0]) || $_[0];
}

sub generate($)
{
	my $self = shift;
	return if exists $self->{type};

	if (defined $self->{value})
	{
		$self->{type} = Ng::Constant->new($self->{value});
	}
	elsif (defined $self->{ref})
	{
		my $d = $self->{ref}->getTarget();
		$d->generate();
		$self->{type} = $d->getDataType();
		unless($self->{type})
		{
			print Dumper([$d]), $/;
			die "Invalid attribute $self->{alias} ref!";
		}
	}
	elsif (defined $self->{data})
	{
		$self->{type} = $self->{data};
	}
	elsif (defined $self->{grub})
	{
		my $g = $self->{grub};
		$g->generate();
		$self->{type} = $g->getDataType();
	}
	else
	{
		print Dumper([$self]), $/;
		die "Cannot generate the attribute type!";
	}
}

sub getDataType($)
{
	my $self = shift;
	return $self->{type};
}

sub getMember($)
{
	my $self = shift;
	my $t = $self->{type};
	return undef unless $t && $t->can('getCppType');

	return {name => main::getCppNameString($self->{name}), cppType => $t->getCppType()};
}

sub getMarshal($;$)
{
	my ($self, $namespace, $t) = @_;
	if ($self->{type}->can('getQualifiedName'))
	{
		$t = $self->{type}->getQualifiedName();
	}
	else
	{
		$t = $self->{type}->getName();
	}
	if (exists $self->{ns})
	{
		return {name => '', cppType => 'Attribute<'.$t.', Name::Scoped<'.$self->{name}.', '.$self->{ns}.'> >'};
	}
	return {name => '', cppType => 'Attribute<'.$t.', Name::Strict<'.$self->{name}.'> >'};
}

package Ng::Empty;
sub new($$)
{
	return bless {}, ref($_[0]) || $_[0]; 
}

sub generate($)
{
}

sub getMarshal($)
{
	my $self = shift;
	return {name => '', cppType => 'Empty'};
}

package Ng::Text;

sub new($$)
{
#	my $n = main::getCppNameString(Strings->generate('ownValue'));
	my $n = main::getCppNameString(Strings->reverse('ownValue'));
	return bless {type => $_[1], name => $n}, ref($_[0]) || $_[0]; 
}

sub generate($)
{
}

sub getMember($)
{
	my $self = shift;
	return undef unless $self->{type}->can('getCppType');
	return {name => $self->{name}, cppType => $self->{type}->getCppType()};
}

sub getMarshal($)
{
	my $self = shift;
	my $t = $self->{type};
	my $n = $t->can('getQualifiedName') ? $t->getQualifiedName() : $t->getName();
	return {name => '', cppType => "Text<${n} >"};
}

sub getDataType($)
{
	my $self = shift;
	return $self->{type};
}

package Ng::Cocoon;

sub new($$)
{
	my $output = bless $_[1], ref($_[0]) || $_[0];
	$output->{type} = Ng::DataType->new('QDomElement');
	return $output;
}

sub generate($)
{
}

sub getMember($)
{
	my $self = shift;
	return {name => '', cppType => 'QDomElement'};
}

sub getMarshal($)
{
	my $self = shift;
	return {name => '', cppType => "Pod"};
}

sub getDataType($)
{
	my $self = shift;
	return $self->{type};
}

package Ng::Block;
use Data::Dumper;

sub new($$$;$)
{
	die "An ARRAY reference is required!" unless ref($_[2]) eq 'ARRAY';
	my $output = {name => $_[1], children => $_[2]};
	$output->{data} = $_[3] if @_ > 3 && $_[3];
	$output->{inlineOdinOptional} = 1;

	return bless $output, ref($_[0]) || $_[0]; 
}

sub setInlineOdinOptional($$)
{
	my $self = shift;
	$self->{inlineOdinOptional} = shift;
}

sub getBody($)
{
	my $self = shift;
	my @z = @{$self->{children}};
	while (@z)
	{
		my $f = 0;
		my @output = ();
		foreach my $x (@z)
		{
			unless ($x->can('getTarget'))
			{
				$x->generate();
				push @output, $x;
				next;
			}
			$f = 1;
			my $d = $x->getTarget();
			$d->generate();
			if ($d->getType())
			{
				push @output, $d;
			}
			else
			{
				push @output, $d->getBody();
			}
		}
		return @output unless $f;

		@z = @output;
	}
	return ();
}

sub getData($)
{
	my $self = shift;
	return $self->{pod} if exists $self->{pod};
	return undef if exists $self->{type} || exists $self->{inline};

	my @z = grep {$_}
		map {$_->generate(); $_->getData()}
		map {$_->getTarget()}
		grep {$_->can('getTarget')} @{$self->{children}};

	@z or return undef;
	die "Too many datas!" if @z > 1;

	return shift @z;
}

sub getDataType($)
{
	my $self = shift;
	my ($d, $t, $i);
	$d = $self->getData() and return $d;
	$t = $self->getType() and return $t;
	$i = $self->getInline() or die "Achtung!!!11";

	return $i->can('getDataType') ? $i->getDataType() : undef;
}

sub getType($)
{
	my $self = shift;
	return $self->{type};
}

sub getInline($)
{
	my $self = shift;
	return $self->{inline} if exists $self->{inline};
	return Ng::Text->new($self->{pod}) if exists $self->{pod};

	return undef;
}

sub getInlineGroup()
{
	my $self = shift;
	return exists($self->{group}) ? $self->{group} : undef;
}

sub generate($)
{
	my $self = shift;
	return if exists $self->{generated};

	$self->{generated} = 1;
	my @b = $self->getBody();
	my $d = $self->{data} || $self->getData();
	unless (@b)
	{
		if ($d)
		{
			$self->{pod} = $d;
		}
		else
		{
			$self->{inline} = Ng::Empty->new();
		}
		return;
	}
	## NB. the body is not empty thus it cannot be inside an attribute.
	if ($d && !(1 == @b && $b[0]->can('getDataType') && $b[0]->getDataType() == $d))
	{
		## NB. add text if $d is not a replacement for $b[0].
		push @b, Ng::Text->new($d);
	}
	my ($o, $g) = shift @b;
	if (@b)
	{
		$g = Ng::Group->ordered([$o, @b]);
	}
	elsif ($o->can('flatten'))
	{
		$g = $o;
	}
	elsif ($o->can('getType'))
	{
		$self->{type} = $o->getType();
		return;
	}
	else
	{
		if ($o->can('getData'))
		{
			my $p = $o->getData();
			if ($p)
			{
				$self->{pod} = $p;
				return;
			}
		}
		my $s = Strings->lookup($self->{name});
		unless ('Ng::Optional' eq ref($o) && $s && !$self->{inlineOdinOptional})
		{
			$self->{inline} = $o;
			return;
		}
		$g = Ng::Group->ordered([$o]);
	}
	my @f = $g->flatten();
	my @m = grep {$_->{member}->can('getDataType') && ref($_->{member}->getDataType()) ne 'Ng::Constant'} @f;
	if (1 == scalar(@m) && (@b || $o->can('flatten')))
	{
		# some entries don't produce a type.
		$self->{group} = $g if scalar(@f) > 1;
		$self->{inline} = $m[0]->{member};
	}
	else
	{
		my $n = $self->{name} || Strings->generate('anonymous');
		$self->{type} = Factory->buildComplexType($n, [@m], $g);
	}
}

package Ng::Element;
use Data::Dumper;
use UNIVERSAL qw(isa);

sub new($$)
{
        return bless $_[1], ref($_[0]) || $_[0];
}

sub getNameTag($)
{
	my $self = shift;
	if (exists $self->{ns})
	{
		return 'Name::Scoped<'.$self->{name}.', '.$self->{ns}.'>';
	}
	return 'Name::Strict<'.$self->{name}.'>';
}

sub generate($)
{
	my $self = shift;
	return if exists $self->{block};

	my $b = Ng::Block->new($self->{name}, $self->{children});
	$b->setInlineOdinOptional(0);
	$b->generate();
	my $t = $b->getType();
	if ($t)
	{
		$t->setSerializer(Ng::Serializer::Element->new($t, $self->getNameTag()));
	}
	$self->{block} = $b;
}

sub getMember($)
{
	my $self = shift;
	my $b = $self->{block};
	my $output = {name => main::getCppNameString($self->{name})};
	if ($b->getInline())
	{
		my $f = $b->getInline();
		if ($f->can('getMember'))
		{
			my $m = $f->getMember() or return undef;
			$output->{cppType} = $m->{cppType};
		}
		else
		{
			$output->{cppType} = Ng::Pod->new('boolean')->getCppType();
		}
	}
	else
	{
		$output->{cppType} = $b->getType()->getCppType();
	}
	return $output;
}

sub getMarshal($;$)
{
	my ($self, $namespace) = @_;
	my ($b, $t) = $self->{block};
	if ($b->getInlineGroup())
	{
		$t = $b->getInlineGroup()->getMarshal($namespace)->{cppType};
	}
	elsif ($b->getInline())
	{
		my $i = $b->getInline();
		$t = $i->getMarshal($namespace)->{cppType};
		# TODO. wrap to text if the inline has a pod
		# type.
		if ($i->can('getDataType'))
		{
#			print STDERR ref($i->getDataType()), ' I', $/;
		}
	}
	else
	{
		$t = $b->getType()->getQualifiedName();
	}
	$t or $t = '';
	return {name => '', cppType => 'Element<'.$t.', '.$self->getNameTag().' >'};
}

sub getDataType($)
{
	my $self = shift;
	return $self->{block}->getDataType();
}

package Ng::Choice;
use Data::Dumper;

sub new($$$$)
{
	my ($z, $n, $v, $b) = @_;
        return bless {name => $n, values => $v, blocks => $b}, ref($z) || $z;
}

sub generate($)
{
	my $self = shift;
	return if exists $self->{generated};

	$self->{generated} = 1;
	if (@{$self->{values}})
	{
		$self->{enum} = Factory->buildEnum($self->{name}, $self->{values});
	}
	my ($p, $v) = 1;
	if (@{$self->{blocks}})
	{
		$v = Factory->beginVariant($self->{name}, ());
		$v->addAlternative($self->{enum}) if exists $self->{enum};
		$self->{variants} = $v;
	}
	elsif (!exists($self->{enum}))
	{
		die "Choice without variants!";
	}
	foreach my $b (@{$self->{blocks}})
	{
		$p = 0;
		$b->generate();
		if ($b->getData())
		{
			my $d = $b->getData();
			$v->addAlternative($d);
			$p = 1;
		}
		elsif ($b->getType())
		{
			my $t = $b->getType();
			$v->addAlternative($t);
		}
		elsif ($b->getInlineGroup())
		{
			my $g = $b->getInlineGroup();
			$v->addAlternative($g);
		}
		elsif ($b->getInline())
		{
			my $i = $b->getInline();
			if ($i->can('getDataType') && ref($i->getDataType()) eq 'Ng::Constant')
			{
				my $n = Strings->generate(Strings->lookup($self->{name}));
				my $t = Factory->buildComplexType($n, [], $i);
				$v->addAlternative($t);
			}
			else
			{
				$v->addAlternative($i);
			}
		}
		else
		{
			die "Unexpected variant!";
		}
	}
	if ($p)
	{
		if ($v)
		{
			$v->setTraits(Ng::Variant::Traits->new());
			Factory->endDataVariant($v);
		}
		$self->{data} = $self->getDataType();
	}
	elsif ($v)
	{
		Factory->endComplexVariant($v);
	}
}

sub getData($)
{
	my $self = shift;
	return $self->{data};
}

sub getEnum($)
{
	my $self = shift;
	return $self->{enum} if exists $self->{enum};

	return undef;
}

sub getDataType($)
{
	my $self = shift;
	return $self->{variants} if exists $self->{variants};

	return $self->getEnum();
}

sub getMember($)
{
	my $self = shift;
	my $t = $self->getDataType();
	unless ($t)
	{
		print Dumper($self), $/, Strings->lookup($self->{name}), $/;
		die "Choice without a data type!";
	}
	return {name => main::getCppNameString($self->{name}), cppType => $t->getCppType()};
}

sub getMarshal($;$)
{
	my ($self, $namespace) = @_;
	if (exists $self->{variants})
	{
		return {name => '', cppType => $self->{variants}->getQualifiedName().'Impl'};
	}
	return Ng::Text->new($self->getEnum())->getMarshal($namespace);
}

package Ng::Serializer::Element;

sub new($$)
{
	my ($z, $t, $n) = @_;
	my $output = {name => $n, type => $t->{name}};
	return bless $output, ref($z) || $z;
}

sub getLoader($$)
{
	my ($self, $src) = @_;
	my @output;
	push @output, "QStack<QDomElement> k;";
	push @output, "k.push(${src});";
	push @output, "Element<$self->{type}, $self->{name} > m;";
	push @output, "if (0 > m.consume(k))";
	push @output, "\treturn false;";
	push @output, '';
	push @output, "*this = m.getValue();";
	push @output, "return true;";
	return @output;
}

sub getSaviour($$)
{
	my ($self, $dst) = @_;
	my @output;
	push @output, "Element<$self->{type}, $self->{name} > m;";
	push @output, "m.setValue(*this);";
	push @output, "return 0 <= m.produce(${dst});";
	return @output;
}

sub getTopSaviour($$)
{
	my ($self, $dst) = @_;
	my @output;
	push @output, "Element<$self->{type}, $self->{name} > m;";
	push @output, "m.setValue(*this);";
	push @output, "return 0 <= m.produce(${dst});";
	return @output;
}

package Ng::Serializer::Fragment;

sub new($$)
{
	my ($z, $t) = @_;
	my $output = {type => $t->{name}};
	return bless $output, ref($z) || $z;
}

sub getLoader($$)
{
	my ($self, $src) = @_;
	my @output;
	push @output, "QStack<QDomElement> k;";
	push @output, "k.push(${src});";
	push @output, "k.push(${src}.firstChildElement());";
	push @output, "return 0 <= Traits<$self->{type}>::parse(*this, k);";
	return @output;
}

sub getSaviour($$)
{
	my ($self, $dst) = @_;
	return ("return 0 <= Traits<$self->{type}>::generate(*this, ${dst});");
}

package main;

our $parser = XML::LibXML->new();
our $first;
our $start;
our %defines;
our %types;
our @names;
our %includes;

sub parse($)
{
	my ($p, $n) = @_;
	for ($n = $p->firstChild(); $n; $n = $n->nextSibling())
	{
		next unless $n->nodeType == XML_ELEMENT_NODE;
		if ('define' eq $n->nodeName) {
			my $d = parseDefine($n);
			$defines{getName($n)} = $d;
		} elsif ('start' eq $n->nodeName) {
			$start = parseElement($n);
		} elsif ('include' eq $n->nodeName) {
			parseInclude($n);
		}
	}
}

sub getNs($)
{
	my $a = $_[0]->getAttributeNode('ns');
	return undef unless $a;
	return $a->value;
}

sub getName($)
{
	my $a = $_[0]->getAttributeNode('name');
	return undef unless $a;
	return $a->value;
}

sub getCppNameString($)
{
	my $output = ucfirst(Strings->lookup(shift));
	$output =~ s![/_.-]+(\w)!uc($1)!ge;
	return $output;
}

sub parseRef($)
{
	return Ng::Ref->new(getName($_[0]));
}

sub parseValue($)
{
	return Strings->reverse($_[0]->textContent());
}

sub parseData($$)
{
	my ($p, $a, $n) = @_;
	my ($t, $f, $x) = ($p->getAttributeNode('type')->value, {}, []);
	for ($n = $p->firstChild(); $n; $n = $n->nextSibling())
	{
		next unless $n->nodeType == XML_ELEMENT_NODE;
		if ('param' eq $n->nodeName) {
			$f->{getName($n)} = $n->firstChild->data;
		} elsif ('except' eq $n->nodeName) {
			push @$x, $n->firstChild->data;
		} else {
			die "Unsupported data child! ".$n->nodeName;
		}
	}
	return Factory->buildData($t, getCppNameString(Strings->reverse($a)), $f, $x);
}

sub parseAttributeChoice($$)
{
	my ($p, $a, $n) = @_;
	my $t = {values => [], refs => [], data => []};
	for ($n = $p->firstChild(); $n; $n = $n->nextSibling())
	{
		next unless $n->nodeType == XML_ELEMENT_NODE;
		if ('value' eq $n->nodeName) {
			push @{$t->{values}}, parseValue($n);
		} elsif ('ref' eq $n->nodeName) {
			push @{$t->{refs}}, parseRef($n);
		} elsif ('data' eq $n->nodeName) {
			push @{$t->{data}}, parseData($n, $a->{alias});
		} else {
			die "Unsupported attribute choice child! ".$n->nodeName;
		}
	}
	if (0 == scalar(@{$t->{refs}}) && !defined($t->{data}) && scalar(@{$t->{values}}) == 1)
	{
		$a->{value} = $t->{values}[0];
	}
	else
	{
		$a->{grub} = $t;
	}
}

sub parseText($)
{
	return Factory->buildData('token');
}

sub parseAttribute($)
{
	my ($p, $n) = @_;
	my $a = getName($p);
	my $output = {	name	=> Strings->reverse($a),
			alias	=> $a,
			value	=> undef,
			data	=> undef,
			grub	=> undef,
			ref	=> undef};
	my $s = getNs($p);
	if ($n = $p->firstChild())
	{
		for (; $n; $n = $n->nextSibling())
		{
			next unless $n->nodeType == XML_ELEMENT_NODE;
			if ('ref' eq $n->nodeName) {
				$output->{ref} = parseRef($n);
			} elsif ('data' eq $n->nodeName) {
				$output->{data} = parseData($n, $a);
			} elsif ('text' eq $n->nodeName) {
				$output->{data} = parseText($n);
			} elsif ('choice' eq $n->nodeName) {
#				parseAttributeChoice($n, $output);
				$output->{grub} = parseChoice($n, $output->{name});
			} elsif ('value' eq $n->nodeName) {
				$output->{value} = parseValue($n);
			}
		}
	}
	else
	{
		$output->{data} = parseText(undef);
	}
	$s and $output->{ns} = Strings->reverse($s);
	return Ng::Attribute->new($output);
}

sub parseAttributableFragment($)
{
	my ($p, $n) = @_;
	my $output = {	children => [],
			text => undef};
	for ($n = $p->firstChild(); $n; $n = $n->nextSibling())
	{
		next unless $n->nodeType == XML_ELEMENT_NODE;
		if ('element' eq $n->nodeName) {
			push @{$output->{children}}, parseElement($n);
		} elsif ('attribute' eq $n->nodeName) {
			push @{$output->{children}}, parseAttribute($n);
		} elsif ('ref' eq $n->nodeName) {
			push @{$output->{children}}, parseRef($n);
		} elsif ('text' eq $n->nodeName) {
			$output->{text} = parseText($n);
		} elsif ('empty' eq $n->nodeName) {
		} elsif ('optional' eq $n->nodeName) {
			push @{$output->{children}}, parseOptional($n);
		} elsif ('zeroOrMore' eq $n->nodeName) {
			push @{$output->{children}}, parseZeroOrMore($n);
		} elsif ('oneOrMore' eq $n->nodeName) {
			push @{$output->{children}}, parseOneOrMore($n);
		} elsif ('group' eq $n->nodeName) {
			push @{$output->{children}}, parseOrdered($n);
		} elsif ('interleave' eq $n->nodeName) {
			push @{$output->{children}}, parseUnordered($n);
		} elsif ('choice' eq $n->nodeName) {
			push @{$output->{children}}, parseChoice($n);
		} elsif ('value' eq $n->nodeName) {
		} elsif ('data' eq $n->nodeName) {
		} else {
			die "Unsupported attributable fragment child! ".$n->nodeName;
		}
	}
	return $output;
}

sub parseFragment($)
{
	my ($p, $n) = @_;
	my $output = {	children => [],
			text => undef};
	for ($n = $p->firstChild(); $n; $n = $n->nextSibling())
	{
		next unless $n->nodeType == XML_ELEMENT_NODE;
		if ('element' eq $n->nodeName) {
			push @{$output->{children}}, parseElement($n);
		} elsif ('ref' eq $n->nodeName) {
			push @{$output->{children}}, parseRef($n);
		} elsif ('text' eq $n->nodeName) {
			$output->{text} = parseText($n);
		} elsif ('empty' eq $n->nodeName) {
		} elsif ('optional' eq $n->nodeName) {
			push @{$output->{children}}, parseOptional($n);
		} elsif ('zeroOrMore' eq $n->nodeName) {
			push @{$output->{children}}, parseZeroOrMore($n);
		} elsif ('oneOrMore' eq $n->nodeName) {
			push @{$output->{children}}, parseOneOrMore($n);
		} elsif ('group' eq $n->nodeName) {
			push @{$output->{children}}, parseOrdered($n);
		} elsif ('interleave' eq $n->nodeName) {
			push @{$output->{children}}, parseUnordered($n);
		} elsif ('choice' eq $n->nodeName) {
			push @{$output->{children}}, parseChoice($n);
		} else {
			die "Unsupported fragment child! ".$n->nodeName;
		}
	}
	return $output;
}

sub parseOptional($)
{
	return Ng::Optional->new(parseAttributableFragment($_[0]));
}

sub parseZeroOrMore($)
{
	return Ng::OrMore->zero(parseFragment($_[0]));
}

sub parseOneOrMore($)
{
	return Ng::OrMore->one(parseFragment($_[0]));
}

sub parseOrdered($)
{
	my $b = parseAttributableFragment($_[0])->{children};
	unless (1 < scalar(@$b))
	{
#		return $b->[0];
	}
	return Ng::Group->ordered($b);
}

sub parseUnordered($)
{
	my $b = parseAttributableFragment($_[0])->{children};
	return $b->[0] if scalar(@$b) == 1;

	return Ng::Group->unordered($b);
}

sub parseGrammar($)
{
	my ($p, $n) = @_;
	for ($n = $p->firstChild(); $n; $n = $n->nextSibling())
	{
		next unless $n->nodeType == XML_ELEMENT_NODE;
		if ('include' eq $n->nodeName)
		{
			parseInclude($n);
		        my $h = $n->getAttributeNode('href');
			my $i = $includes{$h->value};
			next unless 'ARRAY' eq ref($i) && scalar(@$i) == 1;
			return $i->[0];
		}
	}
	return undef;
}

sub parseChoice($;$)
{
	my ($p, $k, $n, @b, @v) = @_;
	my $a = Strings->lookup($k) if $k && $k > 0;
	for ($n = $p->firstChild(); $n; $n = $n->nextSibling())
	{
		next unless $n->nodeType == XML_ELEMENT_NODE;
		if ('value' eq $n->nodeName)
		{
			push @v, parseValue($n);
			next;
		}
		my (@o, $d);
		if ('ref' eq $n->nodeName)
		{
			push @o, parseRef($n);
		}
		elsif ('element' eq $n->nodeName)
		{
			push @o, parseElement($n);
		}
		elsif ('attribute' eq $n->nodeName)
		{
			push @o, parseAttribute($n);
		}
		elsif ('text' eq $n->nodeName)
		{
			$d = parseText($n);
		}
		elsif ('empty' eq $n->nodeName)
		{
			push @o, Ng::Empty->new();
			next;
		}
		elsif ('optional' eq $n->nodeName)
		{
			push @o, parseOptional($n);
		}
		elsif ('zeroOrMore' eq $n->nodeName)
		{
			push @o, parseZeroOrMore($n);
		}
		elsif ('oneOrMore' eq $n->nodeName)
		{
			push @o, parseOneOrMore($n);
		}
		elsif ('group' eq $n->nodeName)
		{
			push @o, parseOrdered($n);
		}
		elsif ('interleave' eq $n->nodeName)
		{
			push @o, parseUnordered($n);
		}
		elsif ('choice' eq $n->nodeName)
		{
			push @o, parseChoice($n);
		}
		elsif ('data' eq $n->nodeName)
		{
			$d = parseData($n, Strings->lookup(Strings->generate('data')));
		}
		elsif ('grammar' eq $n->nodeName)
		{
			my $d = parseGrammar($n);
			Factory->addDependency($d->getInline()->{block}->getType());
			push @b, $d;
			next;
		}
		die "Unsupported choice child! ".$n->nodeName unless @o || $d;
		push @b, Ng::Block->new(Strings->generate($a or 'variant'), [@o], $d);
	}
	return Ng::Choice->new($a ? $k : Strings->generate('choice'), [@v], [@b]) if @v || @b > 1;

	for ($n = $p->firstChild(); $n; $n = $n->nextSibling())
	{
		next unless $n->nodeType == XML_ELEMENT_NODE;
		my (@o, $d);
		if ('ref' eq $n->nodeName)
		{
			return parseRef($n);
		}
		elsif ('element' eq $n->nodeName)
		{
			return parseElement($n);
		}
		elsif ('attribute' eq $n->nodeName)
		{
			return parseAttribute($n);
		}
		elsif ('text' eq $n->nodeName)
		{
			return Ng::Text->new(parseText($n));
		}
		elsif ('empty' eq $n->nodeName)
		{
			return Ng::Empty->new();
		}
		elsif ('optional' eq $n->nodeName)
		{
			return parseOptional($n);
		}
		elsif ('zeroOrMore' eq $n->nodeName)
		{
			return parseZeroOrMore($n);
		}
		elsif ('oneOrMore' eq $n->nodeName)
		{
			return parseOneOrMore($n);
		}
		elsif ('group' eq $n->nodeName)
		{
			return parseOrdered($n);
		}
		elsif ('interleave' eq $n->nodeName)
		{
			return parseUordered($n);
		}
		elsif ('choice' eq $n->nodeName)
		{
			return parseChoice($n);
		}
		else
		{
			die "Unexpected!!!11 ".$n->nodeName;
		}
	}
	die "Unexpected!!!!111";
}

sub parseElement($)
{
	my ($p, $n) = @_;
	my $s = getNs($p);
	my $a = getName($p);
	my $b = 0;
	my $output = {	name => Strings->reverse($a),
			children => [],
			value => undef,
			text => undef};
	for ($n = $p->firstChild(); $n; $n = $n->nextSibling())
	{
		next unless $n->nodeType == XML_ELEMENT_NODE;
		if ('element' eq $n->nodeName) {
			push @{$output->{children}}, parseElement($n);
		} elsif ('attribute' eq $n->nodeName) {
			push @{$output->{children}}, parseAttribute($n);
		} elsif ('ref' eq $n->nodeName) {
			push @{$output->{children}}, parseRef($n);
		} elsif ('text' eq $n->nodeName) {
			push @{$output->{children}}, Ng::Text->new(parseText($n));
		} elsif ('empty' eq $n->nodeName) {
		} elsif ('optional' eq $n->nodeName) {
			push @{$output->{children}}, parseOptional($n);
		} elsif ('zeroOrMore' eq $n->nodeName) {
			push @{$output->{children}}, parseZeroOrMore($n);
		} elsif ('oneOrMore' eq $n->nodeName) {
			push @{$output->{children}}, parseOneOrMore($n);
		} elsif ('group' eq $n->nodeName) {
			push @{$output->{children}}, parseOrdered($n);
		} elsif ('interleave' eq $n->nodeName) {
			push @{$output->{children}}, parseUnordered($n);
		} elsif ('choice' eq $n->nodeName) {
			push @{$output->{children}}, parseChoice($n, $output->{name});
#		} elsif ('notAllowed' eq $n->nodeName) {
		} elsif ('anyName' eq $n->nodeName) {
			$b = 1;
		} elsif ('value' eq $n->nodeName) {
			push @{$output->{children}}, Ng::Text->new(Ng::Constant->new(parseValue($n)));
		} elsif ('data' eq $n->nodeName) {
			push @{$output->{children}}, Ng::Text->new(parseData($n, $a));
		} else {
			die "Unsupported element child! ".$n->nodeName;
		}
	}
	$s and $output->{ns} = Strings->reverse($s);
	return $b ? Ng::Cocoon->new($output) : Ng::Element->new($output);
}

sub parseDefine($)
{
	my ($p, $n) = @_;
	my $a = getName($p);
	my ($k, $c, $d) = (Strings->reverse($a), []);
	for ($n = $p->firstChild(); $n; $n = $n->nextSibling())
	{
		next unless $n->nodeType == XML_ELEMENT_NODE;
		if ('element' eq $n->nodeName) {
			push @$c, parseElement($n);
		} elsif ('attribute' eq $n->nodeName) {
			push @$c, parseAttribute($n);
		} elsif ('ref' eq $n->nodeName) {
			push @$c, parseRef($n);
		} elsif ('text' eq $n->nodeName) {
			$d = parseText($n);
		} elsif ('empty' eq $n->nodeName) {
		} elsif ('optional' eq $n->nodeName) {
			push @$c, parseOptional($n);
		} elsif ('zeroOrMore' eq $n->nodeName) {
			push @$c, parseZeroOrMore($n);
		} elsif ('oneOrMore' eq $n->nodeName) {
			push @$c, parseOneOrMore($n);
		} elsif ('group' eq $n->nodeName) {
			push @$c, parseOrdered($n);
		} elsif ('interleave' eq $n->nodeName) {
			push @$c, parseUnordered($n);
		} elsif ('choice' eq $n->nodeName) {
			push @$c, parseChoice($n, $k);
		} elsif ('notAllowed' eq $n->nodeName) {
		} elsif ('anyName' eq $n->nodeName) {
		} elsif ('value' eq $n->nodeName) {
			die "Top level value inside the define!";
#			$output->{value} = parseValue($n);
		} elsif ('data' eq $n->nodeName) {
			$d = parseData($n, $a);
		} else {
			die "Unsupported element child! ".$n->nodeName;
		}
	}
	return Ng::Block->new($k, $c, $d);
}

sub parseInclude_($)
{
	my ($p, $n) = @_;
	for ($n = $p->firstChild(); $n; $n = $n->nextSibling())
	{
		next unless $n->nodeType == XML_ELEMENT_NODE;
		if ('define' eq $n->nodeName) {
			my $d = parseDefine($n);
			$defines{getName($n)} = $d;
		} elsif ('include' eq $n->nodeName) {
			parseInclude($n);
		} elsif ('start' eq $n->nodeName) {
		} else {
			die "Unsupported include child! ".$n->nodeName;
		}
	}
}

sub parseInclude($)
{
	my $e = shift;
	my $h = $e->getAttributeNode('href');
	my $x = File::Spec->rel2abs($h->value, dirname($first));
	my $d = $parser->parse_file($x);
	parseInclude_($d->documentElement());
}

sub generate
{
	foreach my $t (@_)
	{
		my $d = $defines{$t};
		foreach my $x (@{$d->{children}})
		{
			if (isa $x, 'Ng::Ref')
			{
			}
			elsif (isa $x, 'Ng::Element')
			{
				$x->generate();
	#			map {print $_, $/} $x->{block}{type}->getTraits();
	#			map {print $_, $/} $x->{block}{inline}->getDataType()->getDefinition();
	#			map {print $_, $/} $x->{block}{type}->getDefinition();
			}
			else
			{
				$x->generate();
			}
		}
	}
	Factory->produceDatas();
	Factory->produceEnums();
	Factory->produceTypes();
#	print STDERR Dumper($d), $/;
}

my @av = @ARGV;
if (-1 == index($av[0], ':'))
{
	Strings->prime(File::Spec->rel2abs(shift @av));
}
foreach my $x (@av)
{
	my ($rng, $tags, $name) = split /:/, $x;
	Factory->setup($name);
	$first = File::Spec->rel2abs($rng);
	my $d = $parser->parse_file($rng);
	parse($d->documentElement());
	my @g = split /\s*,\s*/, $tags;
print STDERR $/, $/, $/, $/;
	generate(@g);
	$includes{$rng} = [map {$_->generate(); $_} @defines{@g}];
}
Factory->produceTexts();

